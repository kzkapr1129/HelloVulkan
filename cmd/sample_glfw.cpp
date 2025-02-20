#include "sample_glfw.h"

#include <iostream>
#include <vulkan/vulkan.hpp> // vulkanのインクルードが先
#include <GLFW/glfw3.h>

const uint32_t screenWidth = 640;
const uint32_t screenHeight = 480;

int SampleGLFW::execute() {
    // GLFWの初期化
    if (!glfwInit())
        return -1;

    // GLFWがVulkanをサポートしているか確認
    if (glfwVulkanSupported() == GLFW_FALSE) {
        std::cout << "Vulkan is not supported" << std::endl;
        return -1;
    }

    uint32_t requiredExtensionsCount;
    // サーフェース関連の拡張機能名を取得する
    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);
    std::cout << "Extensions:" << std::endl;
    for (int i = 0; i < requiredExtensionsCount; i++) {
        std::cout << "\t" << requiredExtensions[i] << std::endl;
    }

    vk::InstanceCreateInfo createInfo;
    // Vulkanインスタンスに拡張機能を指定する
    createInfo.enabledExtensionCount = requiredExtensionsCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions;

    // Vulkanのインスタンス作成
    vk::UniqueInstance instance;
    std::cout << "create instance..." << std::endl;
    instance = vk::createInstanceUnique(createInfo);

    // GLFWでウインドウを作成する際のおまじない
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window;
    std::cout << "create window..." << std::endl;
    // ウインドウの作成
    window = glfwCreateWindow(screenWidth, screenHeight, "GLFW Test Window", NULL, NULL);
    if (!window) {
        const char* err;
        glfwGetError(&err);
        std::cout << "(glfwCreateWindow)" << err << std::endl;
        glfwTerminate();
        return -1;
    }

    VkSurfaceKHR c_surface;
    // サーフェースの作成
    auto result = glfwCreateWindowSurface(instance.get(), window, nullptr, &c_surface);
    if (result != VK_SUCCESS) {
        const char* err;
        glfwGetError(&err);
        std::cout << err << std::endl;
        glfwTerminate();
        return -1;
    }
    // C言語版VulkanをVulkan-Hpp形式に変換する
    vk::UniqueSurfaceKHR surface{ c_surface, instance.get() };

    // 物理デバイスの列挙
    std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    uint32_t graphicsQueueFamilyIndex;

    // 物理デバイスの剪定
    for (size_t i = 0; i < physicalDevices.size(); i++) {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();
        bool existsGraphicsQueue = false;

        for (size_t j = 0; j < queueProps.size(); j++) {
            // グラフィックキューがサポートしているか確認
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics) {
                existsGraphicsQueue = true;
                graphicsQueueFamilyIndex = j;
                break;
            }
        }

        // 物理デバイスがサポートしている拡張機能を取得
        std::vector<vk::ExtensionProperties> extProps = physicalDevices[i].enumerateDeviceExtensionProperties();
        bool supportsSwapchainExtension = false;

        for (size_t j = 0; j < extProps.size(); j++) {
            // スワップチェーンをサポートしているか確認
            if (std::string_view(extProps[j].extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
                supportsSwapchainExtension = true;
                break;
            }
        }

        bool supportsSurface =
            !physicalDevices[i].getSurfaceFormatsKHR(surface.get()).empty() ||
            !physicalDevices[i].getSurfacePresentModesKHR(surface.get()).empty();

        if (existsGraphicsQueue && supportsSwapchainExtension && supportsSurface) {
            physicalDevice = physicalDevices[i];
            existsSuitablePhysicalDevice = true;
            break;
        }
    }

    if (!existsSuitablePhysicalDevice) {
        std::cerr << "使用可能な物理デバイスがありません。" << std::endl;
        return -1;
    }

    vk::DeviceCreateInfo devCreateInfo;
    // 物理デバイスで使用する拡張機能として、スワップチェーンを指定
    auto devRequiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    devCreateInfo.enabledExtensionCount = devRequiredExtensions.size();
    devCreateInfo.ppEnabledExtensionNames = devRequiredExtensions.begin();

    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].queueCount = 1;

    float queuePriorities[1] = { 1.0 };

    queueCreateInfo[0].pQueuePriorities = queuePriorities;

    devCreateInfo.pQueueCreateInfos = queueCreateInfo;
    devCreateInfo.queueCreateInfoCount = 1;

    // 論理デバイスの作成
    vk::UniqueDevice device = physicalDevice.createDeviceUnique(devCreateInfo);

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface.get());
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());

    vk::SurfaceFormatKHR swapchainFormat = surfaceFormats[0];
    vk::PresentModeKHR swapchainPresentMode = surfacePresentModes[0];

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    // 画像の出力先となるサーフェースを指定
    swapchainCreateInfo.surface = surface.get();
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1; // 2枚以上の画像を使用することを宣言する (ダブルバッファ)
    swapchainCreateInfo.imageFormat = swapchainFormat.format; // 画像フォーマット
    swapchainCreateInfo.imageColorSpace = swapchainFormat.colorSpace;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent; // 画像サイズ
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    // スワップチェーン(所謂ダブルバッファのこと。ただし、画像は二枚固定ではくminImageCountに応じて増やせる)の作成
    vk::UniqueSwapchainKHR swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);

    std::cout << "wait for close ..." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

#include "sample_glfw.h"

#include <iostream>
#include <vulkan/vulkan.hpp> // vulkanのインクルードが先
#include <GLFW/glfw3.h>

const uint32_t screenWidth = 640;
const uint32_t screenHeight = 480;

int SampleGLFW::execute() {
    if (!glfwInit())
        return -1;

    if (glfwVulkanSupported() == GLFW_FALSE) {
        std::cout << "Vulkan is not supported" << std::endl;
        return -1;
    }

    uint32_t requiredExtensionsCount;
    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);
    std::cout << "Extensions:" << std::endl;
    for (int i = 0; i < requiredExtensionsCount; i++) {
        std::cout << "\t" << requiredExtensions[i] << std::endl;
    }

    vk::InstanceCreateInfo createInfo;
    createInfo.enabledExtensionCount = requiredExtensionsCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions;

    vk::UniqueInstance instance;
    std::cout << "create instance..." << std::endl;
    instance = vk::createInstanceUnique(createInfo);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window;
    std::cout << "create window..." << std::endl;
    window = glfwCreateWindow(screenWidth, screenHeight, "GLFW Test Window", NULL, NULL);
    if (!window) {
        const char* err;
        glfwGetError(&err);
        std::cout << "(glfwCreateWindow)" << err << std::endl;
        glfwTerminate();
        return -1;
    }

    VkSurfaceKHR c_surface;
    auto result = glfwCreateWindowSurface(instance.get(), window, nullptr, &c_surface);
    if (result != VK_SUCCESS) {
        const char* err;
        glfwGetError(&err);
        std::cout << err << std::endl;
        glfwTerminate();
        return -1;
    }
    vk::UniqueSurfaceKHR surface{ c_surface, instance.get() };

    std::cout << "wait for close ..." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

#include "sample_glfw.h"

#include <iostream>
#include <vulkan/vulkan.hpp> // vulkanのインクルードが先
#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>

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
        std::cout << err << std::endl;
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
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics &&
                physicalDevices[i].getSurfaceSupportKHR(j, surface.get())) {
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

    // グラフィックキューの取得
    vk::Queue graphicsQueue = device->getQueue(graphicsQueueFamilyIndex, 0);

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

    // スワップチェーンから画像を取得
    std::vector<vk::Image> swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

    vk::AttachmentDescription attachments[1];
    attachments[0].format = swapchainFormat.format; // スワップチェーンと同じフォーマットを指定する
    attachments[0].samples = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference subpass0_attachmentRefs[1];
    subpass0_attachmentRefs[0].attachment = 0;
    subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpasses[1];
    subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpass0_attachmentRefs;

    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = attachments;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = subpasses;
    renderpassCreateInfo.dependencyCount = 0;
    renderpassCreateInfo.pDependencies = nullptr;

    vk::UniqueRenderPass renderpass = device->createRenderPassUnique(renderpassCreateInfo);

    vk::Viewport viewports[1];
    viewports[0].x = 0.0;
    viewports[0].y = 0.0;
    viewports[0].minDepth = 0.0;
    viewports[0].maxDepth = 1.0;
    viewports[0].width = screenWidth;
    viewports[0].height = screenHeight;

    vk::Rect2D scissors[1];
    scissors[0].offset = vk::Offset2D{ 0, 0 };
    scissors[0].extent = vk::Extent2D{ screenWidth, screenHeight };

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = viewports;
    viewportState.scissorCount = 1;
    viewportState.pScissors = scissors;

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = false;

    vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.sampleShadingEnable = false;
    multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState blendattachment[1];
    blendattachment[0].colorWriteMask =
        vk::ColorComponentFlagBits::eA |
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB;
    blendattachment[0].blendEnable = false;

    vk::PipelineColorBlendStateCreateInfo blend;
    blend.logicOpEnable = false;
    blend.attachmentCount = 1;
    blend.pAttachments = blendattachment;

    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = nullptr;

    vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique(layoutCreateInfo);

    size_t vertSpvFileSz = std::filesystem::file_size("../shader/shader.vert.spv");

    std::ifstream vertSpvFile("../shader/shader.vert.spv", std::ios_base::binary);

    std::vector<char> vertSpvFileData(vertSpvFileSz);
    vertSpvFile.read(vertSpvFileData.data(), vertSpvFileSz);

    vk::ShaderModuleCreateInfo vertShaderCreateInfo;
    vertShaderCreateInfo.codeSize = vertSpvFileSz;
    vertShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertSpvFileData.data());

    vk::UniqueShaderModule vertShader = device->createShaderModuleUnique(vertShaderCreateInfo);

    size_t fragSpvFileSz = std::filesystem::file_size("../shader/shader.frag.spv");

    std::ifstream fragSpvFile("../shader/shader.frag.spv", std::ios_base::binary);

    std::vector<char> fragSpvFileData(fragSpvFileSz);
    fragSpvFile.read(fragSpvFileData.data(), fragSpvFileSz);

    vk::ShaderModuleCreateInfo fragShaderCreateInfo;
    fragShaderCreateInfo.codeSize = fragSpvFileSz;
    fragShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragSpvFileData.data());

    vk::UniqueShaderModule fragShader = device->createShaderModuleUnique(fragShaderCreateInfo);

    vk::PipelineShaderStageCreateInfo shaderStage[2];
    shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStage[0].module = vertShader.get();
    shaderStage[0].pName = "main";
    shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStage[1].module = fragShader.get();
    shaderStage[1].pName = "main";

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisample;
    pipelineCreateInfo.pColorBlendState = &blend;
    pipelineCreateInfo.layout = pipelineLayout.get();
    pipelineCreateInfo.renderPass = renderpass.get();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.stageCount = 2;
    pipelineCreateInfo.pStages = shaderStage;

    vk::UniquePipeline pipeline = device->createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;

    std::vector<vk::UniqueImageView> swapchainImageViews(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = swapchainImages[i];
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imgViewCreateInfo.format = swapchainFormat.format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        swapchainImageViews[i] = device->createImageViewUnique(imgViewCreateInfo);
    }

    std::vector<vk::UniqueFramebuffer> swapchainFramebufs(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageView frameBufAttachments[1];
        frameBufAttachments[0] = swapchainImageViews[i].get();

        vk::FramebufferCreateInfo frameBufCreateInfo;
        frameBufCreateInfo.width = surfaceCapabilities.currentExtent.width;
        frameBufCreateInfo.height = surfaceCapabilities.currentExtent.height;
        frameBufCreateInfo.layers = 1;
        frameBufCreateInfo.renderPass = renderpass.get();
        frameBufCreateInfo.attachmentCount = 1;
        frameBufCreateInfo.pAttachments = frameBufAttachments;

        swapchainFramebufs[i] = device->createFramebufferUnique(frameBufCreateInfo);
    }

    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::UniqueCommandPool cmdPool = device->createCommandPoolUnique(cmdPoolCreateInfo);

    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmdBufs =
        device->allocateCommandBuffersUnique(cmdBufAllocInfo);

    vk::FenceCreateInfo fenceCreateInfo;
    vk::UniqueFence swapchainImgFence = device->createFenceUnique(fenceCreateInfo);

    std::cout << "wait for close ..." << std::endl;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        device->resetFences({ swapchainImgFence.get() });

        // スワップチェーンから次の画像を取得する (タイムアウト時間の単位 1'000'000'000ナノ秒=>1秒)
        vk::ResultValue acquireImgResult = device->acquireNextImageKHR(swapchain.get(), 1'000'000'000, {}, swapchainImgFence.get());
        if (acquireImgResult.result != vk::Result::eSuccess) {
            std::cerr << "次フレームの取得に失敗しました。" << std::endl;
            return -1;
        }
        uint32_t imgIndex = acquireImgResult.value;

        if (device->waitForFences({ swapchainImgFence.get() }, VK_TRUE, 1'000'000'000) != vk::Result::eSuccess) {
            std::cerr << "次フレームの取得に失敗しました。" << std::endl;
            return -1;
        }

        /**
         * スワップチェーンによって画像が切り替わるため、
         * コマンドバッファをリセットして記録し直す
         */
        cmdBufs[0]->reset();

        vk::CommandBufferBeginInfo cmdBeginInfo;
        cmdBufs[0]->begin(cmdBeginInfo);

        vk::ClearValue clearVal[1];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        vk::RenderPassBeginInfo renderpassBeginInfo;
        renderpassBeginInfo.renderPass = renderpass.get();
        renderpassBeginInfo.framebuffer = swapchainFramebufs[imgIndex].get();
        renderpassBeginInfo.renderArea = vk::Rect2D({ 0,0 }, { screenWidth, screenHeight });
        renderpassBeginInfo.clearValueCount = 1;
        renderpassBeginInfo.pClearValues = clearVal;

        cmdBufs[0]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

        cmdBufs[0]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        cmdBufs[0]->draw(3, 1, 0, 0);

        cmdBufs[0]->endRenderPass();

        cmdBufs[0]->end();

        vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = submitCmdBuf;
        graphicsQueue.submit({ submitInfo }, nullptr);

        graphicsQueue.waitIdle();

        vk::PresentInfoKHR presentInfo;

        auto presentSwapchains = { swapchain.get() };
        auto imgIndices = { imgIndex };

        presentInfo.swapchainCount = presentSwapchains.size();
        presentInfo.pSwapchains = presentSwapchains.begin();
        presentInfo.pImageIndices = imgIndices.begin();

        // プレゼンテーション(ウインドウに画像を表示)のコマンドをGPUに送信する
        graphicsQueue.presentKHR(presentInfo);

        graphicsQueue.waitIdle();
    }

    glfwTerminate();
    return 0;
}

#include "index_buffer.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

const uint32_t screenWidth = 640;
const uint32_t screenHeight = 480;

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;
};

struct Vertex {
    Vec2 pos;
    Vec3 color;
};

static std::vector<Vertex> vertices = {
    Vertex{ Vec2{-0.5f, -0.5f }, Vec3{ 0.0, 0.0, 1.0 } },
    Vertex{ Vec2{ 0.5f,  0.5f }, Vec3{ 0.0, 1.0, 0.0 } },
    Vertex{ Vec2{-0.5f,  0.5f }, Vec3{ 1.0, 0.0, 0.0 } },
    Vertex{ Vec2{ 0.5f, -0.5f }, Vec3{ 1.0, 1.0, 1.0 } },
};

static std::vector<uint16_t> indices = { 0, 1, 2, 1, 0, 3 };


int IndexBuffer::execute() {
    if (!glfwInit())
        return -1;

    uint32_t requiredExtensionsCount;
    const char** requiredExtensions = glfwGetRequiredInstanceExtensions(&requiredExtensionsCount);

    vk::InstanceCreateInfo createInfo;
    createInfo.enabledExtensionCount = requiredExtensionsCount;
    createInfo.ppEnabledExtensionNames = requiredExtensions;

    vk::UniqueInstance instance;
    instance = vk::createInstanceUnique(createInfo);

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window;
    window = glfwCreateWindow(screenWidth, screenHeight, "GLFW Test Window", NULL, NULL);
    if (!window) {
        const char* err;
        glfwGetError(&err);
        std::cout << err << std::endl;
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

    std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    uint32_t graphicsQueueFamilyIndex;

    for (size_t i = 0; i < physicalDevices.size(); i++) {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();
        bool existsGraphicsQueue = false;

        for (size_t j = 0; j < queueProps.size(); j++) {
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics &&
                physicalDevices[i].getSurfaceSupportKHR(j, surface.get())) {
                existsGraphicsQueue = true;
                graphicsQueueFamilyIndex = j;
                break;
            }
        }

        std::vector<vk::ExtensionProperties> extProps = physicalDevices[i].enumerateDeviceExtensionProperties();
        bool supportsSwapchainExtension = false;

        for (size_t j = 0; j < extProps.size(); j++) {
            if (std::string_view(extProps[j].extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) {
                supportsSwapchainExtension = true;
                break;
            }
        }

        if (existsGraphicsQueue && supportsSwapchainExtension) {
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

    vk::UniqueDevice device = physicalDevice.createDeviceUnique(devCreateInfo);

    vk::Queue graphicsQueue = device->getQueue(graphicsQueueFamilyIndex, 0);

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::BufferCreateInfo vertBufferCreateInfo;
    vertBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    vertBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer;
    vertBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::UniqueBuffer vertexBuf = device->createBufferUnique(vertBufferCreateInfo);

    vk::MemoryRequirements vertexBufMemReq = device->getBufferMemoryRequirements(vertexBuf.get());
        
    vk::MemoryAllocateInfo vertexBufMemAllocInfo;
    vertexBufMemAllocInfo.allocationSize = vertexBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (vertexBufMemReq.memoryTypeBits & (1 << i) &&
            (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        return -1;
    }

    vk::UniqueDeviceMemory vertexBufMemory = device->allocateMemoryUnique(vertexBufMemAllocInfo);

    device->bindBufferMemory(vertexBuf.get(), vertexBufMemory.get(), 0);

    void* vertexBufMem = device->mapMemory(vertexBufMemory.get(), 0, sizeof(Vertex) * vertices.size());

    std::memcpy(vertexBufMem, vertices.data(), sizeof(Vertex) * vertices.size());

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = vertexBufMemory.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(Vertex) * vertices.size();

    device->flushMappedMemoryRanges({ flushMemoryRange });

    device->unmapMemory(vertexBufMemory.get());

    vk::BufferCreateInfo indexBufferCreateInfo;
    indexBufferCreateInfo.size = sizeof(uint16_t) * indices.size();
    indexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer; // インデックスバッファを指定
    indexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::UniqueBuffer indexBuf = device->createBufferUnique(indexBufferCreateInfo);

    vk::MemoryRequirements indexBufMemReq = device->getBufferMemoryRequirements(indexBuf.get());

    vk::MemoryAllocateInfo indexBufMemAllocInfo;
    indexBufMemAllocInfo.allocationSize = indexBufMemReq.size;

    suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (indexBufMemReq.memoryTypeBits & (1 << i) &&
            (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) {
            indexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        return -1;
    }

    vk::UniqueDeviceMemory indexBufMemory = device->allocateMemoryUnique(indexBufMemAllocInfo);

    device->bindBufferMemory(indexBuf.get(), indexBufMemory.get(), 0);

    void* indexBufMem = device->mapMemory(indexBufMemory.get(), 0, sizeof(uint16_t) * indices.size());

    std::memcpy(indexBufMem, indices.data(), sizeof(uint16_t) * indices.size());

    flushMemoryRange.memory = indexBufMemory.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(uint16_t) * indices.size();

    device->flushMappedMemoryRanges({ flushMemoryRange });

    device->unmapMemory(indexBufMemory.get());

    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface.get());
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());

    vk::SurfaceFormatKHR swapchainFormat = surfaceFormats[0];
    vk::PresentModeKHR swapchainPresentMode = surfacePresentModes[0];

    vk::AttachmentDescription attachments[1];
    attachments[0].format = swapchainFormat.format;
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

    vk::VertexInputBindingDescription vertexBindingDescription[1];
    vertexBindingDescription[0].binding = 0;
    vertexBindingDescription[0].stride = sizeof(Vertex);
    vertexBindingDescription[0].inputRate = vk::VertexInputRate::eVertex;

    vk::VertexInputAttributeDescription vertexInputDescription[2];
    vertexInputDescription[0].binding = 0;
    vertexInputDescription[0].location = 0;
    vertexInputDescription[0].format = vk::Format::eR32G32Sfloat;
    vertexInputDescription[0].offset = offsetof(Vertex, pos);
    vertexInputDescription[1].binding = 0;
    vertexInputDescription[1].location = 1;
    vertexInputDescription[1].format = vk::Format::eR32G32B32Sfloat;
    vertexInputDescription[1].offset = offsetof(Vertex, color);

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = std::size(vertexBindingDescription);
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = std::size(vertexInputDescription);
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputDescription;

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

    size_t vertSpvFileSz = std::filesystem::file_size("../shader/shader.vert2.spv");

    std::ifstream vertSpvFile("../shader/shader.vert2.spv", std::ios_base::binary);

    std::vector<char> vertSpvFileData(vertSpvFileSz);
    vertSpvFile.read(vertSpvFileData.data(), vertSpvFileSz);

    vk::ShaderModuleCreateInfo vertShaderCreateInfo;
    vertShaderCreateInfo.codeSize = vertSpvFileSz;
    vertShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertSpvFileData.data());

    vk::UniqueShaderModule vertShader = device->createShaderModuleUnique(vertShaderCreateInfo);

    size_t fragSpvFileSz = std::filesystem::file_size("../shader/shader.frag2.spv");

    std::ifstream fragSpvFile("../shader/shader.frag2.spv", std::ios_base::binary);

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

    vk::UniqueSwapchainKHR swapchain;
    std::vector<vk::Image> swapchainImages;
    std::vector<vk::UniqueImageView> swapchainImageViews;
    std::vector<vk::UniqueFramebuffer> swapchainFramebufs;

    auto recreateSwapchain = [&](){
        swapchainFramebufs.clear();
        swapchainImageViews.clear();
        swapchainImages.clear();
        swapchain.reset();

        vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());

        vk::SwapchainCreateInfoKHR swapchainCreateInfo;
        swapchainCreateInfo.surface = surface.get();
        swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
        swapchainCreateInfo.imageFormat = swapchainFormat.format;
        swapchainCreateInfo.imageColorSpace = swapchainFormat.colorSpace;
        swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
        swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
        swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
        swapchainCreateInfo.presentMode = swapchainPresentMode;
        swapchainCreateInfo.clipped = VK_TRUE;

        swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);

        swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

        swapchainImageViews.resize(swapchainImages.size());

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

        swapchainFramebufs.resize(swapchainImages.size());

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
    };

    recreateSwapchain();

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
        
    vk::SemaphoreCreateInfo semaphoreCreateInfo;

    vk::UniqueSemaphore swapchainImgSemaphore, imgRenderedSemaphore;
    swapchainImgSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);
    imgRenderedSemaphore = device->createSemaphoreUnique(semaphoreCreateInfo);
    
    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;

    vk::UniqueFence imgRenderedFence = device->createFenceUnique(fenceCreateInfo);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        device->waitForFences({ imgRenderedFence.get()}, VK_TRUE, UINT64_MAX);

        vk::ResultValue acquireImgResult = device->acquireNextImageKHR(swapchain.get(), 1'000'000'000, swapchainImgSemaphore.get());
        if(acquireImgResult.result == vk::Result::eSuboptimalKHR || acquireImgResult.result == vk::Result::eErrorOutOfDateKHR) {
            std::cerr << "スワップチェーンを再作成します。" << std::endl;
            recreateSwapchain();
            continue;
        }
        if (acquireImgResult.result != vk::Result::eSuccess) {
            std::cerr << "次フレームの取得に失敗しました。" << std::endl;
            return -1;
        }
        device->resetFences({ imgRenderedFence.get() });

        uint32_t imgIndex = acquireImgResult.value;

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
        cmdBufs[0]->bindVertexBuffers(0, { vertexBuf.get() }, { 0 });
        cmdBufs[0]->bindIndexBuffer(indexBuf.get(), 0, vk::IndexType::eUint16); // インデックスバッファを使用した描画
        cmdBufs[0]->drawIndexed(indices.size(), 1, 0, 0, 0);

        cmdBufs[0]->endRenderPass();

        cmdBufs[0]->end();

        vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = submitCmdBuf;

        vk::Semaphore renderwaitSemaphores[] = { swapchainImgSemaphore.get() };
        vk::PipelineStageFlags renderwaitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = renderwaitSemaphores;
        submitInfo.pWaitDstStageMask = renderwaitStages;

        vk::Semaphore renderSignalSemaphores[] = { imgRenderedSemaphore.get() };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = renderSignalSemaphores;

        graphicsQueue.submit({ submitInfo }, imgRenderedFence.get());

        vk::PresentInfoKHR presentInfo;

        auto presentSwapchains = { swapchain.get() };
        auto imgIndices = { imgIndex };

        presentInfo.swapchainCount = presentSwapchains.size();
        presentInfo.pSwapchains = presentSwapchains.begin();
        presentInfo.pImageIndices = imgIndices.begin();

        vk::Semaphore presenWaitSemaphores[] = { imgRenderedSemaphore.get() };
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = presenWaitSemaphores;

        graphicsQueue.presentKHR(presentInfo);
    }

    graphicsQueue.waitIdle();
    glfwTerminate();
    return 0;
}
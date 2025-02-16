#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>
#include <fstream>
#include <filesystem>

int main() {
    vk::InstanceCreateInfo createInfo;

    // Vulkanのインスタンス作成
    vk::UniqueInstance instance;
    instance = vk::createInstanceUnique(createInfo);

    // 物理デバイス(PCに刺さっているグラボ)の列挙
    std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    uint32_t graphicsQueueFamilyIndex;

    // グラフィックスキューをサポートしている物理デバイスを探す
    for (size_t i = 0; i < physicalDevices.size(); i++) {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();
        bool existsGraphicsQueue = false;

        // キューファミリーの中にグラフィックスキューがあるかどうかを探す
        for (size_t j = 0; j < queueProps.size(); j++) {
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics) {
                existsGraphicsQueue = true;
                graphicsQueueFamilyIndex = j;
                break;
            }
        }

        if (existsGraphicsQueue) {
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

    //　選択したキューを指定する
    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].queueCount = 1;
    float queuePriorities[1] = { 1.0f };
    queueCreateInfo[0].pQueuePriorities = queuePriorities;

    devCreateInfo.pQueueCreateInfos = queueCreateInfo;
    devCreateInfo.queueCreateInfoCount = 1;

    // 論理デバイスを作成する
    vk::UniqueDevice device = physicalDevice.createDeviceUnique(devCreateInfo);

    // キューを取得する
    vk::Queue graphicsQueue = device->getQueue(graphicsQueueFamilyIndex, 0);

    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    // コマンドプールを作成する
    vk::UniqueCommandPool cmdPool = device->createCommandPoolUnique(cmdPoolCreateInfo);

    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    // コマンドプールからコマンドバッファ(複数のコマンドをまとめるバッファ)を取得する
    std::vector<vk::UniqueCommandBuffer> cmdBufs = device->allocateCommandBuffersUnique(cmdBufAllocInfo);

    // 画像の横幅
    const uint32_t screenWidth = 640;
    // 画像の高さ
    const uint32_t screenHeight = 480;

    vk::ImageCreateInfo imgCreateInfo;
    imgCreateInfo.imageType = vk::ImageType::e2D; // 画像の次元(1〜3次元)
    imgCreateInfo.extent = vk::Extent3D(screenWidth, screenHeight, 1); // 画像サイズ
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm; // 画素のフォーマット
    imgCreateInfo.tiling = vk::ImageTiling::eLinear;
    imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
    imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    // 画像の作成
    vk::UniqueImage image = device->createImageUnique(imgCreateInfo);

    // 画像が要求するメモリサイズを取得する
    vk::MemoryRequirements imgMemReq = device->getImageMemoryRequirements(image.get());

    // デバイスが持っているメモリの種類を取得する
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryAllocateInfo imgMemAllocInfo;
    imgMemAllocInfo.allocationSize = imgMemReq.size;

    // 利用可能なメモリを選択する
    bool suitableMemoryTypeFound = false;
    for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (imgMemReq.memoryTypeBits & (1 << i)) {
            // 一番最初にビットが立っているメモリを使用する
            imgMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }

    if (!suitableMemoryTypeFound) {
        std::cerr << "使用可能なメモリタイプがありません。" << std::endl;
        return -1;
    }

    // デバイスメモリを確保する
    vk::UniqueDeviceMemory imgMem = device->allocateMemoryUnique(imgMemAllocInfo);

    // 画像のメモリを紐づける
    device->bindImageMemory(image.get(), imgMem.get(), 0); // 第三引数の0はメモリの何バイト目から利用するかのアドレス

    // アタッチメントの初期化オブジェクトの用意 (アタッチメント=サブパスで利用するテクスチャだったり、描画対象の画像として利用される)
    vk::AttachmentDescription attachments[1];
    attachments[0].format = vk::Format::eR8G8B8A8Unorm;
    attachments[0].samples = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::eGeneral;

    // アタッチメントとサブパスの関連付けオブジェクトの準備
    vk::AttachmentReference subpass0_attachmentRefs[1];
    subpass0_attachmentRefs[0].attachment = 0;
    subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    // サブパスの初期化オブジェクトの用意 (レンダーパスを構成する要素。一つ以上指定する)
    vk::SubpassDescription subpasses[1];
    subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpass0_attachmentRefs; // サブパスが利用するアタッチメントを指定する

    // レンダーパスの初期化オブジェクトの用意
    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = attachments;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = subpasses;
    renderpassCreateInfo.dependencyCount = 0;
    renderpassCreateInfo.pDependencies = nullptr;

    // レンダーパスを作成する
    vk::UniqueRenderPass renderpass = device->createRenderPassUnique(renderpassCreateInfo);

    // ビューポートの初期化オブジェクトの用意
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

    // バーテックスシェーダーモジュールを作成する
    vk::UniqueShaderModule vertShader = device->createShaderModuleUnique(vertShaderCreateInfo);
    std::cout << "done vert" << std::endl;

    size_t fragSpvFileSz = std::filesystem::file_size("../shader/shader.frag.spv");

    std::ifstream fragSpvFile("../shader/shader.frag.spv", std::ios_base::binary);

    std::vector<char> fragSpvFileData(fragSpvFileSz);
    fragSpvFile.read(fragSpvFileData.data(), fragSpvFileSz);

    vk::ShaderModuleCreateInfo fragShaderCreateInfo;
    fragShaderCreateInfo.codeSize = fragSpvFileSz;
    fragShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragSpvFileData.data());

    // フラグメントシェーダーモジュールを作成
    vk::UniqueShaderModule fragShader = device->createShaderModuleUnique(fragShaderCreateInfo);
    std::cout << "done frag" << std::endl;

    // パイプラインとシェーダーの関連付け用オブジェクトを用意する
    vk::PipelineShaderStageCreateInfo shaderStage[2];
    shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStage[0].module = vertShader.get();
    shaderStage[0].pName = "main";
    shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStage[1].module = fragShader.get();
    shaderStage[1].pName = "main";

    // パイプラインの初期化オブジェクトを用意する
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

    // パイプラインを作成する
    vk::UniquePipeline pipeline = device->createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;

    vk::CommandBufferBeginInfo cmdBeginInfo;
    // コマンドを記録を開始する
    cmdBufs[0]->begin(cmdBeginInfo);

    // コマンドを記録

    // コマンド記録を終了する
    cmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    // コマンドバッファをキューに送信する
    graphicsQueue.submit({ submitInfo }, nullptr);

    // キューがからになるまで待つ
    graphicsQueue.waitIdle();

    return 0;
}
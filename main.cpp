#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>

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
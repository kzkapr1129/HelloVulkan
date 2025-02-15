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
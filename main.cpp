#include <vulkan/vulkan.hpp>

int main() {
     vk::InstanceCreateInfo instCreateInfo;

     vk::UniqueInstance instance;
     instance = vk::createInstanceUnique(instCreateInfo);

     std::vector<vk::PhysicalDevice> physicalDevices = instance->enumeratePhysicalDevices();

     vk::PhysicalDevice physicalDevice = physicalDevices[0];

    return 0;
}
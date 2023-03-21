#include "vulkan_physical_device.h"

namespace progressia::desktop {

PhysicalDevice::PhysicalDevice(VkPhysicalDevice vk) : vk(vk) {
    vkGetPhysicalDeviceProperties(vk, &properties);
    vkGetPhysicalDeviceFeatures(vk, &features);
    vkGetPhysicalDeviceMemoryProperties(vk, &memory);
}

bool PhysicalDevice::isSuitable() const {
    // Add feature, limit, etc. checks here.
    // Return false and debug() if problems arise.
    return true;
}

VkPhysicalDevice PhysicalDevice::getVk() const { return vk; }

const VkPhysicalDeviceProperties &PhysicalDevice::getProperties() const {
    return properties;
}

const VkPhysicalDeviceFeatures &PhysicalDevice::getFeatures() const {
    return features;
}

const VkPhysicalDeviceLimits &PhysicalDevice::getLimits() const {
    return properties.limits;
}

const VkPhysicalDeviceMemoryProperties &PhysicalDevice::getMemory() const {
    return memory;
}

VkPhysicalDeviceType PhysicalDevice::getType() const {
    return properties.deviceType;
}

const char *PhysicalDevice::getName() const { return properties.deviceName; }

VkDeviceSize PhysicalDevice::getMinUniformOffset() const {
    return getLimits().minUniformBufferOffsetAlignment;
}

uint32_t PhysicalDevice::getMaxTextureSize() const {
    return getLimits().maxImageDimension2D;
}

} // namespace progressia::desktop

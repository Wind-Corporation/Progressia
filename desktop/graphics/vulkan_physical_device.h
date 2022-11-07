#pragma once

#include "vulkan_common.h"

namespace progressia {
namespace desktop {

class PhysicalDevice {

  private:
    VkPhysicalDevice vk;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memory;

  public:
    PhysicalDevice(VkPhysicalDevice vk);

    bool isSuitable() const;

    VkPhysicalDevice getVk() const;
    const VkPhysicalDeviceProperties &getProperties() const;
    const VkPhysicalDeviceFeatures &getFeatures() const;
    const VkPhysicalDeviceLimits &getLimits() const;
    const VkPhysicalDeviceMemoryProperties &getMemory() const;

    VkPhysicalDeviceType getType() const;
    const char *getName() const;

    VkDeviceSize getMinUniformOffset() const;
    uint32_t getMaxTextureSize() const;
};

} // namespace desktop
} // namespace progressia

#pragma once

#include "vulkan_common.h"

#include <vector>

namespace progressia {
namespace desktop {

struct PhysicalDeviceData {
    VkPhysicalDevice device;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
};

const PhysicalDeviceData &
pickPhysicalDevice(std::vector<PhysicalDeviceData> &, Vulkan &,
                   const std::vector<const char *> &deviceExtensions);

} // namespace desktop
} // namespace progressia

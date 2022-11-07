#pragma once

#include "vulkan_common.h"
#include "vulkan_physical_device.h"

#include <vector>

namespace progressia {
namespace desktop {

const PhysicalDevice &
pickPhysicalDevice(std::vector<PhysicalDevice> &, Vulkan &,
                   const std::vector<const char *> &deviceExtensions);

} // namespace desktop
} // namespace progressia

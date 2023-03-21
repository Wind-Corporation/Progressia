#pragma once

#include "vulkan_common.h"
#include "vulkan_physical_device.h"

#include <vector>

namespace progressia::desktop {

const PhysicalDevice &
pickPhysicalDevice(std::vector<PhysicalDevice> &, Vulkan &,
                   const std::vector<const char *> &deviceExtensions);

} // namespace progressia::desktop

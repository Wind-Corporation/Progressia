#pragma once

#include "vulkan_common.h"

namespace progressia {
namespace desktop {

void initializeVulkan();

Vulkan *getVulkan();

void resizeVulkanSurface();

/*
 * Returns false when the frame should be skipped
 */
bool startRender();
void endRender();

void shutdownVulkan();

} // namespace desktop
} // namespace progressia

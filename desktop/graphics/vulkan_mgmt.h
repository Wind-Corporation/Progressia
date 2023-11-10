#pragma once

#include "vulkan_common.h"

namespace progressia::desktop {

class VulkanManager {

  private:
    std::unique_ptr<Vulkan> vulkan;

  public:
    VulkanManager();
    ~VulkanManager();

    Vulkan *getVulkan();
    const Vulkan *getVulkan() const;

    void resizeSurface();

    /*
     * Returns false when the frame should be skipped
     */
    bool startRender();
    void endRender();
};

} // namespace progressia::desktop

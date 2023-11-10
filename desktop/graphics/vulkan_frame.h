#pragma once

#include "vulkan_common.h"

namespace progressia::desktop {

class Frame : public VkObjectWrapper {
  private:
    Vulkan &vulkan;

    VkCommandBuffer commandBuffer;

    VkSemaphore imageAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;

    std::vector<VkClearValue> clearValues;

    std::optional<uint32_t> imageIndexInFlight;

  public:
    Frame(Vulkan &vulkan);
    ~Frame();

    /*
     * Returns false when the frame should be skipped
     */
    bool startRender();
    void endRender();

    VkCommandBuffer getCommandBuffer();
};

} // namespace progressia::desktop

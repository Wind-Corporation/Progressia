#pragma once

#include "vulkan_common.h"

namespace progressia::desktop {

class RenderPass : public VkObjectWrapper {

  private:
    VkRenderPass vk;

    Vulkan &vulkan;

  public:
    RenderPass(Vulkan &);
    ~RenderPass();

    VkRenderPass getVk();
};

} // namespace progressia::desktop

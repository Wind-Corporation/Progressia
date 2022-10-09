#pragma once

#include "vulkan_common.h"

namespace progressia {
namespace desktop {

class RenderPass : public VkObjectWrapper {

  private:
    VkRenderPass vk;

    Vulkan &vulkan;

  public:
    RenderPass(Vulkan &);
    ~RenderPass();

    VkRenderPass getVk();
};

} // namespace desktop
} // namespace progressia

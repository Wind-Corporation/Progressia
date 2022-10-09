#pragma once

#include "vulkan_common.h"

namespace progressia {
namespace desktop {

class Pipeline : public VkObjectWrapper {

  private:
    VkPipelineLayout layout;
    VkPipeline vk;

    Vulkan &vulkan;

    VkShaderModule createShaderModule(const std::vector<char> &bytecode);

  public:
    Pipeline(Vulkan &);
    ~Pipeline();

    VkPipeline getVk();
    VkPipelineLayout getLayout();
};

} // namespace desktop
} // namespace progressia

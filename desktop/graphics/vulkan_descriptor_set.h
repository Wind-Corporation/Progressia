#pragma once

#include "vulkan_common.h"

namespace progressia {
namespace desktop {

class DescriptorSetInterface : public VkObjectWrapper {
  protected:
    VkDescriptorSetLayout layout;
    uint32_t setNumber;
    Vulkan &vulkan;

    DescriptorSetInterface(uint32_t setNumber, Vulkan &);

  public:
    VkDescriptorSetLayout getLayout() const;
    uint32_t getSetNumber() const;
    Vulkan &getVulkan();
};

} // namespace desktop
} // namespace progressia

#include "vulkan_descriptor_set.h"

namespace progressia::desktop {

DescriptorSetInterface::DescriptorSetInterface(uint32_t setNumber,
                                               Vulkan &vulkan)
    : setNumber(setNumber), vulkan(vulkan) {}

VkDescriptorSetLayout DescriptorSetInterface::getLayout() const {
    return layout;
}

uint32_t DescriptorSetInterface::getSetNumber() const { return setNumber; }

Vulkan &DescriptorSetInterface::getVulkan() { return vulkan; }

} // namespace progressia::desktop

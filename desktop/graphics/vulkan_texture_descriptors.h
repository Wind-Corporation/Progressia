#pragma once

#include <vector>

#include "vulkan_common.h"
#include "vulkan_descriptor_set.h"

namespace progressia::desktop {

class TextureDescriptors : public DescriptorSetInterface {
  private:
    constexpr static uint32_t POOL_SIZE = 64;
    constexpr static uint32_t SET_NUMBER = 1;

    std::vector<VkDescriptorPool> pools;
    uint32_t lastPoolCapacity;

    void allocatePool();

  public:
    TextureDescriptors(Vulkan &);
    ~TextureDescriptors();

    VkDescriptorSet addTexture(VkImageView, VkSampler);
};

} // namespace progressia::desktop

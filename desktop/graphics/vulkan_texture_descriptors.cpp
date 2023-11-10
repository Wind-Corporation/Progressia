#include "vulkan_texture_descriptors.h"

namespace progressia::desktop {

void TextureDescriptors::allocatePool() {
    pools.resize(pools.size() + 1);

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = POOL_SIZE;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = POOL_SIZE;

    auto *output = &pools[pools.size() - 1];
    vulkan.handleVkResult(
        "Could not create texture descriptor pool",
        vkCreateDescriptorPool(vulkan.getDevice(), &poolInfo, nullptr, output));

    lastPoolCapacity = POOL_SIZE;
}

TextureDescriptors::TextureDescriptors(Vulkan &vulkan)
    : DescriptorSetInterface(SET_NUMBER, vulkan), lastPoolCapacity(0) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    VkDescriptorSetLayoutBinding binding = {};
    binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    binding.pImmutableSamplers = nullptr;
    binding.binding = 0;

    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &binding;

    vulkan.handleVkResult("Could not create texture descriptor set layout",
                          vkCreateDescriptorSetLayout(vulkan.getDevice(),
                                                      &layoutInfo, nullptr,
                                                      &layout));

    allocatePool();
}

TextureDescriptors::~TextureDescriptors() {
    for (auto *pool : pools) {
        vkDestroyDescriptorPool(vulkan.getDevice(), pool, nullptr);
    }

    vkDestroyDescriptorSetLayout(vulkan.getDevice(), layout, nullptr);
}

VkDescriptorSet TextureDescriptors::addTexture(VkImageView view,
                                               VkSampler sampler) {

    /*
     * Allocate descriptor set
     */

    if (lastPoolCapacity == 0) {
        allocatePool();
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pools.back();
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &layout;

    VkDescriptorSet descriptorSet = nullptr;
    vulkan.handleVkResult("Could not create texture descriptor set",
                          vkAllocateDescriptorSets(vulkan.getDevice(),
                                                   &allocInfo, &descriptorSet));

    lastPoolCapacity--;

    /*
     * Write to descriptor set
     */

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(vulkan.getDevice(), 1, &write, 0, nullptr);

    return descriptorSet;
}

} // namespace progressia::desktop

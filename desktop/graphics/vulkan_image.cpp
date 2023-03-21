#include "vulkan_image.h"

#include <cstring>
#include <iostream>

#include "vulkan_buffer.h"
#include "vulkan_common.h"
#include "vulkan_frame.h"
#include "vulkan_pipeline.h"
#include "vulkan_texture_descriptors.h"

namespace progressia::desktop {

/*
 * Image
 */

Image::Image(VkImage vk, VkImageView view, VkFormat format)
    : vk(vk), view(view), format(format) {
    // do nothing
}

Image::~Image() {
    // do nothing
}

/*
 * ManagedImage
 */

ManagedImage::ManagedImage(std::size_t width, std::size_t height,
                           VkFormat format, VkImageAspectFlags aspect,
                           VkImageUsageFlags usage, Vulkan &vulkan)
    :

      Image(VK_NULL_HANDLE, VK_NULL_HANDLE, format), vulkan(vulkan),

      state{VK_IMAGE_LAYOUT_UNDEFINED, 0, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT} {

    /*
     * Create VkImage
     */

    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = static_cast<uint32_t>(width);
    imageInfo.extent.height = static_cast<uint32_t>(height);
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.flags = 0; // Optional

    vulkan.handleVkResult(
        "Could not create an image",
        vkCreateImage(vulkan.getDevice(), &imageInfo, nullptr, &vk));

    /*
     * Allocate memory
     */

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vulkan.getDevice(), vk, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = vulkan.findMemoryType(
        memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    vulkan.handleVkResult(
        "Could not allocate memory for image",
        vkAllocateMemory(vulkan.getDevice(), &allocInfo, nullptr, &memory));

    /*
     * Bind memory to image
     */

    vkBindImageMemory(vulkan.getDevice(), vk, memory, 0);

    /*
     * Create image view
     */

    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vk;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspect;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    vulkan.handleVkResult(
        "Could not create image view",
        vkCreateImageView(vulkan.getDevice(), &viewInfo, nullptr, &view));
}

ManagedImage::~ManagedImage() {
    vkDestroyImageView(vulkan.getDevice(), view, nullptr);
    vkDestroyImage(vulkan.getDevice(), vk, nullptr);
    vkFreeMemory(vulkan.getDevice(), memory, nullptr);
}

void ManagedImage::transition(State newState) {
    VkCommandBuffer commandBuffer = vulkan.getCommandPool().beginSingleUse();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = state.layout;
    barrier.newLayout = newState.layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vk;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = state.accessMask;
    barrier.dstAccessMask = newState.accessMask;

    vkCmdPipelineBarrier(commandBuffer, state.stageMask, newState.stageMask, 0,
                         0, nullptr, 0, nullptr, 1, &barrier);

    vulkan.getCommandPool().runSingleUse(commandBuffer, true);

    state = newState;
}

/*
 * Texture
 */

Texture::Texture(const progressia::main::Image &src, Vulkan &vulkan)
    :

      ManagedImage(src.width, src.height, VK_FORMAT_R8G8B8A8_SRGB,
                   VK_IMAGE_ASPECT_COLOR_BIT,
                   VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
                   vulkan) {

    /*
     * Create a staging buffer
     */

    Buffer<progressia::main::Image::Byte> stagingBuffer(
        src.getSize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        vulkan);

    /*
     * Transfer pixels to staging buffer
     */

    void *dst = stagingBuffer.map();
    memcpy(dst, src.getData(), src.getSize());
    stagingBuffer.unmap();

    /*
     * Transfer pixels from staging buffer to image
     */

    transition({VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT});

    VkCommandBuffer commandBuffer = vulkan.getCommandPool().beginSingleUse();
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {static_cast<uint32_t>(src.width),
                          static_cast<uint32_t>(src.height), 1};
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer.buffer, vk,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    vulkan.getCommandPool().runSingleUse(commandBuffer, true);

    transition({VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_ACCESS_SHADER_READ_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT});

    /*
     * Create a sampler
     */

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 0;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vulkan.handleVkResult(
        "Could not create texture sampler",
        vkCreateSampler(vulkan.getDevice(), &samplerInfo, nullptr, &sampler));

    /*
     * Create descriptor set
     */

    descriptorSet = vulkan.getTextureDescriptors().addTexture(view, sampler);
}

Texture::~Texture() {
    vkDestroySampler(vulkan.getDevice(), sampler, nullptr);
    // TODO free descriptorSet
}

void Texture::bind() {
    // REPORT_ERROR if getCurrentFrame() == nullptr
    auto commandBuffer = vulkan.getCurrentFrame()->getCommandBuffer();
    auto pipelineLayout = vulkan.getPipeline().getLayout();

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            vulkan.getTextureDescriptors().getSetNumber(), 1,
                            &descriptorSet, 0, nullptr);
}

} // namespace progressia::desktop

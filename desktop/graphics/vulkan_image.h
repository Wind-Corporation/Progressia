#pragma once

#include <boost/core/noncopyable.hpp>
#include <vector>

#include "vulkan_buffer.h"
#include "vulkan_common.h"

#include "../../main/rendering/image.h"

namespace progressia {
namespace desktop {

class Image : public VkObjectWrapper {
  public:
    VkImage vk;
    VkImageView view;
    VkFormat format;

    Image(VkImage, VkImageView, VkFormat);
    virtual ~Image();
};

class ManagedImage : public Image {

  public:
    VkDeviceMemory memory;
    Vulkan &vulkan;

    struct State {
        VkImageLayout layout;
        VkAccessFlags accessMask;
        VkPipelineStageFlags stageMask;
    };

  private:
    State state;

  public:
    ManagedImage(std::size_t width, std::size_t height, VkFormat,
                 VkImageAspectFlags, VkImageUsageFlags, Vulkan &);
    ~ManagedImage();

    void transition(State);
};

class Texture : public ManagedImage {

  public:
    VkSampler sampler;
    VkDescriptorSet descriptorSet;

    Texture(const progressia::main::Image &, Vulkan &vulkan);
    ~Texture();

    void bind();
};

} // namespace desktop
} // namespace progressia

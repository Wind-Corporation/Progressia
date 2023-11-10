#pragma once

#include "vulkan_common.h"
#include "vulkan_descriptor_set.h"
#include "vulkan_image.h"
#include "vulkan_uniform.h"

namespace progressia::desktop {

class Attachment {
  public:
    const char *name;

    VkFormat format;
    VkImageAspectFlags aspect;
    VkImageUsageFlags usage;

    VkImageLayout workLayout;
    VkImageLayout finalLayout;
    VkAttachmentLoadOp loadOp;
    VkAttachmentStoreOp storeOp;

    VkClearValue clearValue;

    std::unique_ptr<Image> image;
};

class Adapter : public VkObjectWrapper {
  public:
    using ViewUniform = Uniform<glm::mat4, glm::mat4>;

    struct Light {
        glm::vec4 color;
        glm::vec4 from;
        float contrast;
        float softness;
    };

    using LightUniform = Uniform<Light>;

  private:
    Vulkan &vulkan;

    ViewUniform viewUniform;
    LightUniform lightUniform;

    std::vector<Attachment> attachments;

  public:
    Adapter(Vulkan &);
    ~Adapter();

    std::vector<Attachment> &getAttachments();

    VkVertexInputBindingDescription getVertexInputBindingDescription();
    std::vector<VkVertexInputAttributeDescription>
    getVertexInputAttributeDescriptions();

    std::vector<char> loadVertexShader();
    std::vector<char> loadFragmentShader();

    ViewUniform::State createView();
    LightUniform::State createLight();

    std::vector<VkDescriptorSetLayout> getUsedDSLayouts() const;
    void onPreFrame();
};

} // namespace progressia::desktop

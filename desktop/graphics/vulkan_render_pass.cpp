#include "vulkan_render_pass.h"

#include "vulkan_adapter.h"
#include "vulkan_common.h"

namespace progressia::desktop {

RenderPass::RenderPass(Vulkan &vulkan) : vk(), vulkan(vulkan) {

    std::vector<VkAttachmentDescription> attachmentDescriptions;
    std::vector<VkAttachmentReference> attachmentReferences;
    VkAttachmentReference depthAttachmentRef{};
    const auto &attachments = vulkan.getAdapter().getAttachments();

    for (std::size_t i = 0; i < attachments.size(); i++) {
        const auto &attachment = attachments[i];
        VkAttachmentDescription *desc = nullptr;
        VkAttachmentReference *ref = nullptr;

        attachmentDescriptions.push_back({});
        desc = &attachmentDescriptions.back();
        if (attachment.aspect == VK_IMAGE_ASPECT_DEPTH_BIT) {
            ref = &depthAttachmentRef;
        } else {
            attachmentReferences.push_back({});
            ref = &attachmentReferences.back();
        }

        desc->format = attachment.image == nullptr ? attachment.format
                                                   : attachment.image->format;
        desc->samples = VK_SAMPLE_COUNT_1_BIT;
        desc->loadOp = attachment.loadOp;
        desc->storeOp = attachment.storeOp;
        desc->stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        desc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        desc->initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        desc->finalLayout = attachment.finalLayout;

        ref->attachment = i;
        ref->layout = attachment.workLayout;
    }

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = attachmentReferences.size();
    subpass.pColorAttachments = attachmentReferences.data();
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount =
        static_cast<uint32_t>(attachmentDescriptions.size());
    renderPassInfo.pAttachments = attachmentDescriptions.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vulkan.handleVkResult(
        "Could not create render pass",
        vkCreateRenderPass(vulkan.getDevice(), &renderPassInfo, nullptr, &vk));
}

RenderPass::~RenderPass() {
    vkDestroyRenderPass(vulkan.getDevice(), vk, nullptr);
}

VkRenderPass RenderPass::getVk() { return vk; }

} // namespace progressia::desktop

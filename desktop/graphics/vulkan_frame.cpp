#include "vulkan_frame.h"

#include <limits>

#include "vulkan_adapter.h"
#include "vulkan_common.h"
#include "vulkan_pipeline.h"
#include "vulkan_render_pass.h"
#include "vulkan_swap_chain.h"

namespace progressia::desktop {

Frame::Frame(Vulkan &vulkan)
    : vulkan(vulkan),
      commandBuffer(vulkan.getCommandPool().allocateMultiUse()) {

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

    vulkan.handleVkResult("Could not create imageAvailableSemaphore",
                          vkCreateSemaphore(vulkan.getDevice(), &semaphoreInfo,
                                            nullptr, &imageAvailableSemaphore));
    vulkan.handleVkResult("Could not create renderFinishedSemaphore",
                          vkCreateSemaphore(vulkan.getDevice(), &semaphoreInfo,
                                            nullptr, &renderFinishedSemaphore));
    vulkan.handleVkResult(
        "Could not create inFlightFence",
        vkCreateFence(vulkan.getDevice(), &fenceInfo, nullptr, &inFlightFence));

    for (const auto &attachment : vulkan.getAdapter().getAttachments()) {
        clearValues.push_back(attachment.clearValue);
    }
}

Frame::~Frame() {
    vulkan.waitIdle();
    vkDestroySemaphore(vulkan.getDevice(), imageAvailableSemaphore, nullptr);
    vkDestroySemaphore(vulkan.getDevice(), renderFinishedSemaphore, nullptr);
    vkDestroyFence(vulkan.getDevice(), inFlightFence, nullptr);
}

bool Frame::startRender() {
    // Wait for frame
    vkWaitForFences(vulkan.getDevice(), 1, &inFlightFence, VK_TRUE, UINT64_MAX);

    // Acquire an image
    imageIndexInFlight = 0;
    VkResult result = vkAcquireNextImageKHR(
        vulkan.getDevice(), vulkan.getSwapChain().getVk(), UINT64_MAX,
        imageAvailableSemaphore, VK_NULL_HANDLE, &*imageIndexInFlight);

    switch (result) {
    case VK_ERROR_OUT_OF_DATE_KHR:
        vulkan.getSwapChain().recreate();
        // Skip this frame, try again later
        return false;
    case VK_SUBOPTIMAL_KHR:
        // Continue as normal
        break;
    default:
        vulkan.handleVkResult("Could not acquire next image", result);
        break;
    }

    vulkan.getAdapter().onPreFrame();

    // Reset command buffer
    vkResetCommandBuffer(commandBuffer, 0);

    // Setup command buffer

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vulkan.handleVkResult("Could not begin recording command buffer",
                          vkBeginCommandBuffer(commandBuffer, &beginInfo));

    auto extent = vulkan.getSwapChain().getExtent();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = vulkan.getRenderPass().getVk();
    renderPassInfo.framebuffer =
        vulkan.getSwapChain().getFramebuffer(*imageIndexInFlight);
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;
    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo,
                         VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                      vulkan.getPipeline().getVk());

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)extent.width;
    viewport.height = (float)extent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = extent;
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    return true;
}

void Frame::endRender() {
    // End command buffer
    vkCmdEndRenderPass(commandBuffer);

    vulkan.handleVkResult("Could not end recording command buffer",
                          vkEndCommandBuffer(commandBuffer));

    // Submit command buffer
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(vulkan.getDevice(), 1, &inFlightFence);
    vulkan.handleVkResult(
        "Could not submit draw command buffer",
        vkQueueSubmit(vulkan.getQueues().getGraphicsQueue().getVk(), 1,
                      &submitInfo, inFlightFence));

    // Present result
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {vulkan.getSwapChain().getVk()};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &*imageIndexInFlight;

    VkResult result = vkQueuePresentKHR(
        vulkan.getQueues().getPresentQueue().getVk(), &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // We're at the end of this frame already, no need to skip
        vulkan.getSwapChain().recreate();
    } else {
        vulkan.handleVkResult("Could not present", result);
    }

    imageIndexInFlight.reset();
}

VkCommandBuffer Frame::getCommandBuffer() { return commandBuffer; }

} // namespace progressia::desktop

#include "vulkan_swap_chain.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "glfw_mgmt_details.h"
#include "vulkan_adapter.h"
#include "vulkan_common.h"
#include "vulkan_render_pass.h"

#include "../../main/logging.h"
using namespace progressia::main::logging;

namespace progressia {
namespace desktop {

SwapChain::SupportDetails
SwapChain::querySwapChainSupport(VkPhysicalDevice device, Vulkan &vulkan) {
    SupportDetails details;
    auto surface = vulkan.getSurface().getVk();

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface,
                                              &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                         nullptr);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount,
                                             details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
                                              &presentModeCount, nullptr);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(
            device, surface, &presentModeCount, details.presentModes.data());
    }

    return details;
}

bool SwapChain::isSwapChainSuitable(const SupportDetails &details) {
    return !details.formats.empty() && !details.presentModes.empty();
}

void SwapChain::create() {
    auto details = querySwapChainSupport(vulkan.getPhysicalDevice(), vulkan);
    auto surfaceFormat = chooseSurfaceFormat(details.formats);
    auto presentMode = choosePresentMode(details.presentModes, true);
    this->extent = chooseExtent(details.capabilities);

    uint32_t imageCount = details.capabilities.minImageCount + 1;
    uint32_t maxImageCount = details.capabilities.maxImageCount;
    if (maxImageCount > 0 && imageCount > maxImageCount) {
        imageCount = maxImageCount;
    }

    // Fill out the createInfo

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = vulkan.getSurface().getVk();

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    createInfo.preTransform = details.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain =
        VK_NULL_HANDLE; // TODO Figure out if this should be used

    // Specify queues

    uint32_t queueFamilyIndices[] = {
        vulkan.getQueues().getGraphicsQueue().getFamilyIndex(),
        vulkan.getQueues().getPresentQueue().getFamilyIndex()};

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    // Create swap chain object

    vulkan.handleVkResult(
        "Could not create swap chain",
        vkCreateSwapchainKHR(vulkan.getDevice(), &createInfo, nullptr, &vk));

    // Store color buffers

    std::vector<VkImage> colorBufferImages;
    vkGetSwapchainImagesKHR(vulkan.getDevice(), vk, &imageCount, nullptr);
    colorBufferImages.resize(imageCount);
    vkGetSwapchainImagesKHR(vulkan.getDevice(), vk, &imageCount,
                            colorBufferImages.data());

    colorBufferViews.resize(colorBufferImages.size());
    for (size_t i = 0; i < colorBufferImages.size(); i++) {
        VkImageViewCreateInfo viewCreateInfo{};
        viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreateInfo.image = colorBufferImages[i];
        viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewCreateInfo.format = surfaceFormat.format;
        viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCreateInfo.subresourceRange.baseMipLevel = 0;
        viewCreateInfo.subresourceRange.levelCount = 1;
        viewCreateInfo.subresourceRange.baseArrayLayer = 0;
        viewCreateInfo.subresourceRange.layerCount = 1;

        vulkan.handleVkResult("Cound not create ImageView",
                              vkCreateImageView(vulkan.getDevice(),
                                                &viewCreateInfo, nullptr,
                                                &colorBufferViews[i]));
    }

    // Create attachment images

    for (auto &attachment : vulkan.getAdapter().getAttachments()) {
        if (attachment.format == VK_FORMAT_UNDEFINED) {
            if (!attachment.image) {
                fatal() << "Attachment " << attachment.name
                        << " format is VK_FORMAT_UNDEFINED but it does not "
                           "have an image";
                // REPORT_ERROR
                exit(1);
            }
            continue;
        }

        attachment.image = std::make_unique<ManagedImage>(
            extent.width, extent.height, attachment.format, attachment.aspect,
            attachment.usage, vulkan);
    }

    // Create framebuffer

    framebuffers.resize(colorBufferViews.size());
    for (size_t i = 0; i < framebuffers.size(); i++) {
        std::vector<VkImageView> attachmentViews;
        for (const auto &attachment : vulkan.getAdapter().getAttachments()) {
            if (&attachment == colorBuffer) {
                attachmentViews.push_back(colorBufferViews[i]);
            } else if (attachment.image) {
                attachmentViews.push_back(attachment.image->view);
            } else {
                fatal() << "Attachment " << attachment.name
                        << " is not colorBuffer but it does not have an image";
                // REPORT_ERROR
                exit(1);
            }
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = vulkan.getRenderPass().getVk();
        framebufferInfo.attachmentCount =
            static_cast<uint32_t>(attachmentViews.size());
        framebufferInfo.pAttachments = attachmentViews.data();
        framebufferInfo.width = extent.width;
        framebufferInfo.height = extent.height;
        framebufferInfo.layers = 1;

        vulkan.handleVkResult("Could not create Framebuffer",
                              vkCreateFramebuffer(vulkan.getDevice(),
                                                  &framebufferInfo, nullptr,
                                                  &framebuffers[i]));
    }
}

VkSurfaceFormatKHR SwapChain::chooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &supported) {
    for (const auto &option : supported) {
        if (option.format == VK_FORMAT_B8G8R8A8_SRGB &&
            option.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return option;
        }
    }

    fatal("No suitable formats available");
    // REPORT_ERROR
    exit(1);
}

bool SwapChain::isTripleBufferingSupported(
    const std::vector<VkPresentModeKHR> &supported) {
    return std::find(supported.begin(), supported.end(),
                     VK_PRESENT_MODE_MAILBOX_KHR) != supported.end();
}

VkPresentModeKHR
SwapChain::choosePresentMode(const std::vector<VkPresentModeKHR> &supported,
                             bool avoidVsync) {
    if (avoidVsync && isTripleBufferingSupported(supported)) {
        return VK_PRESENT_MODE_MAILBOX_KHR;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
SwapChain::chooseExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width !=
        std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    int width, height;
    glfwGetFramebufferSize(getGLFWWindowHandle(), &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width),
                               static_cast<uint32_t>(height)};

    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
}

void SwapChain::destroy() {
    for (auto framebuffer : framebuffers) {
        vkDestroyFramebuffer(vulkan.getDevice(), framebuffer, nullptr);
    }
    framebuffers.clear();

    if (depthBuffer != nullptr) {
        delete depthBuffer;
        depthBuffer = nullptr;
    }

    auto &attachments = vulkan.getAdapter().getAttachments();
    for (auto &attachment : attachments) {
        if (attachment.format != VK_FORMAT_UNDEFINED) {
            attachment.image.reset();
        }
    }

    for (auto colorBufferView : colorBufferViews) {
        vkDestroyImageView(vulkan.getDevice(), colorBufferView, nullptr);
    }
    colorBufferViews.clear();

    if (vk != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(vulkan.getDevice(), vk, nullptr);
        vk = VK_NULL_HANDLE;
    }
}

SwapChain::SwapChain(Vulkan &vulkan)
    : vk(VK_NULL_HANDLE), colorBuffer(nullptr),
      colorBufferViews(), extent{0, 0}, depthBuffer(nullptr), framebuffers(),
      vulkan(vulkan) {
    auto details = querySwapChainSupport(vulkan.getPhysicalDevice(), vulkan);
    auto surfaceFormat = chooseSurfaceFormat(details.formats);

    vulkan.getAdapter().getAttachments().push_back(
        {"Color buffer",

         VK_FORMAT_UNDEFINED,
         0,
         0,

         VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
         VK_ATTACHMENT_LOAD_OP_CLEAR,
         VK_ATTACHMENT_STORE_OP_STORE,

         {{{0.0f, 0.0f, 0.0f, 1.0f}}},

         std::make_unique<Image>(static_cast<VkImage>(VK_NULL_HANDLE),
                                 static_cast<VkImageView>(VK_NULL_HANDLE),
                                 surfaceFormat.format)});

    colorBuffer = &vulkan.getAdapter().getAttachments().back();
}

SwapChain::~SwapChain() {
    destroy();

    auto &attachments = vulkan.getAdapter().getAttachments();
    for (auto it = attachments.begin(); it != attachments.end(); it++) {
        if (&(*it) == colorBuffer) {
            attachments.erase(it);
            colorBuffer = nullptr;
            break;
        }
    }
}

void SwapChain::recreate() {
    vulkan.waitIdle();
    destroy();
    create();
}

VkSwapchainKHR SwapChain::getVk() const { return vk; }

VkFramebuffer SwapChain::getFramebuffer(std::size_t index) const {
    return framebuffers.at(index);
}

VkExtent2D SwapChain::getExtent() const { return extent; }

} // namespace desktop
} // namespace progressia

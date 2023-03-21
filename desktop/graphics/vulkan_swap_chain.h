#pragma once

#include "vulkan_adapter.h"
#include "vulkan_common.h"

namespace progressia::desktop {

class SwapChain : public VkObjectWrapper {

  public:
    struct SupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    static SupportDetails querySwapChainSupport(VkPhysicalDevice device,
                                                Vulkan &vulkan);
    static bool isSwapChainSuitable(const SupportDetails &details);

  private:
    VkSwapchainKHR vk;

    Attachment *colorBuffer;
    std::vector<VkImageView> colorBufferViews;

    VkExtent2D extent;

    Image *depthBuffer;

    std::vector<VkFramebuffer> framebuffers;

    Vulkan &vulkan;

    void create();
    void destroy();

    VkSurfaceFormatKHR
    chooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &);
    bool isTripleBufferingSupported(const std::vector<VkPresentModeKHR> &);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR> &,
                                       bool avoidVsync);
    VkExtent2D chooseExtent(const VkSurfaceCapabilitiesKHR &);

  public:
    SwapChain(Vulkan &);
    ~SwapChain();

    void recreate();

    VkSwapchainKHR getVk() const;
    VkFramebuffer getFramebuffer(std::size_t index) const;
    VkExtent2D getExtent() const;
};

} // namespace progressia::desktop

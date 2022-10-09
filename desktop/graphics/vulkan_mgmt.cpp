#include "vulkan_mgmt.h"

#include "vulkan_common.h"
#include "vulkan_swap_chain.h"

namespace progressia {
namespace desktop {

Vulkan *vulkan;

void initializeVulkan() {
    std::cout << "Vulkan initializing" << std::endl;

    // Instance extensions

    std::vector<const char *> instanceExtensions;
    {
        uint32_t glfwExtensionCount;
        const char **glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        for (std::size_t i = 0; i < glfwExtensionCount; i++) {
            instanceExtensions.push_back(glfwExtensions[i]);
        }

#ifdef VULKAN_ERROR_CHECKING
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    }

    // Device extensions

    std::vector<const char *> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    // Validation layers

    std::vector<const char *> validationLayers{
#ifdef VULKAN_ERROR_CHECKING
        "VK_LAYER_KHRONOS_validation"
#endif
    };

    vulkan = new Vulkan(instanceExtensions, deviceExtensions, validationLayers);

    std::cout << "Vulkan initialized" << std::endl;
}

Vulkan *getVulkan() { return vulkan; }

bool startRender() { return vulkan->startRender(); }

void endRender() { return vulkan->endRender(); }

void resizeVulkanSurface() { vulkan->getSwapChain().recreate(); }

void shutdownVulkan() {
    std::cout << "Vulkan terminating" << std::endl;

    if (vulkan != nullptr) {
        delete vulkan;
        vulkan = nullptr;
    }

    std::cout << "Vulkan terminated" << std::endl;
}

} // namespace desktop
} // namespace progressia

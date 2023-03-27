#include "vulkan_mgmt.h"

#include "vulkan_common.h"
#include "vulkan_swap_chain.h"

#include "../../main/logging.h"
using namespace progressia::main::logging;

namespace progressia::desktop {

VulkanManager::VulkanManager() {
    debug("Vulkan initializing");

    // Instance extensions

    std::vector<const char *> instanceExtensions;
    {
        uint32_t glfwExtensionCount = 0;
        const char **glfwExtensions =
            glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        instanceExtensions.reserve(instanceExtensions.size() +
                                   glfwExtensionCount);
        for (std::size_t i = 0; i < glfwExtensionCount; i++) {
            instanceExtensions.emplace_back(glfwExtensions[i]);
        }

#ifdef VULKAN_ERROR_CHECKING
        instanceExtensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
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

    vulkan = std::make_unique<Vulkan>(instanceExtensions, deviceExtensions,
                                      validationLayers);

    debug("Vulkan initialized");
}

VulkanManager::~VulkanManager() { debug("Vulkan terminating"); }

Vulkan *VulkanManager::getVulkan() { return vulkan.get(); }
const Vulkan *VulkanManager::getVulkan() const { return vulkan.get(); }

bool VulkanManager::startRender() { return vulkan->startRender(); }

void VulkanManager::endRender() { return vulkan->endRender(); }

void VulkanManager::resizeSurface() { vulkan->getSwapChain().recreate(); }

} // namespace progressia::desktop

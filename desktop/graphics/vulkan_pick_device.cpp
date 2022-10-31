#include "vulkan_pick_device.h"

#include "../../main/logging.h"
#include "vulkan_swap_chain.h"
using namespace progressia::main::logging;

namespace progressia {
namespace desktop {

namespace {

bool checkDeviceExtensions(VkPhysicalDevice device,
                           const std::vector<const char *> &deviceExtensions) {
    CstrUtils::CstrHashSet toFind(deviceExtensions.cbegin(),
                                  deviceExtensions.cend());

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         nullptr);

    std::vector<VkExtensionProperties> available(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                         available.data());

    for (const auto &extension : available) {
        toFind.erase(extension.extensionName);
    }

    return toFind.empty();
}

bool isDeviceSuitable(const PhysicalDeviceData &data, Vulkan &vulkan,
                      const std::vector<const char *> &deviceExtensions) {

    if (!Queues(data.device, vulkan).isComplete()) {
        return false;
    }

    if (!checkDeviceExtensions(data.device, deviceExtensions)) {
        return false;
    }

    // Check requires that the swap chain extension is present
    if (!SwapChain::isSwapChainSuitable(
            SwapChain::querySwapChainSupport(data.device, vulkan))) {
        return false;
    }

    return true;
}

} // namespace

const PhysicalDeviceData &
pickPhysicalDevice(std::vector<PhysicalDeviceData> &choices, Vulkan &vulkan,
                   const std::vector<const char *> &deviceExtensions) {

    // Remove unsuitable devices
    auto it = std::remove_if(choices.begin(), choices.end(), [&](auto x) {
        return !isDeviceSuitable(x, vulkan, deviceExtensions);
    });
    choices.erase(it, choices.end());

    if (choices.empty()) {
        fatal("No suitable GPUs found");
        // REPORT_ERROR
        exit(1);
    }

    const auto *pick = &choices.front();

    auto m = info("\n");
    m << "Suitable devices:";
    for (const auto &option : choices) {

        struct {
            const char *description;
            int value;
        } opinions[] = {{"<unknown>", 0},
                        {"Integrated GPU", 0},
                        {"Discrete GPU", +1},
                        {"Virtual GPU", +1},
                        {"CPU", -1}};

        auto type = option.properties.deviceType;
        m << "\n\t- " << opinions[type].description << " "
          << option.properties.deviceName;

        if (opinions[pick->properties.deviceType].value <
            opinions[type].value) {
            pick = &option;
        }
    }
    m << "\n";

    m << "Picked device " << pick->properties.deviceName;
    return *pick;
}

} // namespace desktop
} // namespace progressia

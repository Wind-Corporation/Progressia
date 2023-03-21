#include "vulkan_pick_device.h"

#include "../../main/logging.h"
#include "vulkan_swap_chain.h"
using namespace progressia::main::logging;

namespace progressia::desktop {

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

bool isDeviceSuitable(const PhysicalDevice &data, Vulkan &vulkan,
                      const std::vector<const char *> &deviceExtensions) {

    if (!data.isSuitable()) {
        return false;
    }

    if (!Queues(data.getVk(), vulkan).isComplete()) {
        return false;
    }

    if (!checkDeviceExtensions(data.getVk(), deviceExtensions)) {
        return false;
    }

    // Check requires that the swap chain extension is present
    if (!SwapChain::isSwapChainSuitable(
            SwapChain::querySwapChainSupport(data.getVk(), vulkan))) {
        return false;
    }

    return true;
}

} // namespace

const PhysicalDevice &
pickPhysicalDevice(std::vector<PhysicalDevice> &choices, Vulkan &vulkan,
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

        auto type = option.getType();
        m << "\n\t- " << opinions[type].description << " " << option.getName();

        if (opinions[pick->getType()].value < opinions[type].value) {
            pick = &option;
        }
    }
    m << "\n";

    m << "Picked device " << pick->getName();
    return *pick;
}

} // namespace progressia::desktop

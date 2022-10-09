#include "vulkan_common.h"

#include "vulkan_adapter.h"
#include "vulkan_frame.h"
#include "vulkan_pick_device.h"
#include "vulkan_pipeline.h"
#include "vulkan_render_pass.h"
#include "vulkan_swap_chain.h"
#include "vulkan_texture_descriptors.h"

#include "../../main/meta.h"
#include "glfw_mgmt_details.h"

namespace progressia {
namespace desktop {

/*
 * Vulkan
 */

Vulkan::Vulkan(std::vector<const char *> instanceExtensions,
               std::vector<const char *> deviceExtensions,
               std::vector<const char *> validationLayers)
    :

      frames(MAX_FRAMES_IN_FLIGHT), isRenderingFrame(false),
      lastStartedFrame(0) {

    /*
     * Create error handler
     */
    errorHandler = std::make_unique<VulkanErrorHandler>(*this);

    /*
     * Create instance
     */
    {
        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

        // Set application data

        using namespace progressia::main::meta;

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = NAME;
        appInfo.applicationVersion =
            VK_MAKE_VERSION(VERSION.major, VERSION.minor, VERSION.patch);
        appInfo.pEngineName = nullptr;
        appInfo.engineVersion = 0;
        appInfo.apiVersion = VK_API_VERSION_1_0;
        createInfo.pApplicationInfo = &appInfo;

        // Enable extensions
        {
            uint32_t extensionCount;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                   nullptr);
            std::vector<VkExtensionProperties> available(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount,
                                                   available.data());

            CstrUtils::CstrHashSet toFind(instanceExtensions.cbegin(),
                                          instanceExtensions.cend());

            for (const auto &extensionProperties : available) {
                toFind.erase(extensionProperties.extensionName);
            }

            if (!toFind.empty()) {
                std::cout << "Could not locate following requested Vulkan "
                             "extensions:";
                for (const auto &extension : toFind) {
                    std::cout << "\n\t- " << extension;
                }
                std::cout << std::endl;
                // REPORT_ERROR
                exit(1);
            }
        }

        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(instanceExtensions.size());
        createInfo.ppEnabledExtensionNames = instanceExtensions.data();

        // Enable validation layers
        {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> available(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, available.data());

            CstrUtils::CstrHashSet toFind(validationLayers.cbegin(),
                                          validationLayers.cend());

            for (const auto &layerProperties : available) {
                toFind.erase(layerProperties.layerName);
            }

            if (!toFind.empty()) {
                std::cout << "Could not locate following requested Vulkan "
                             "validation layers:";
                for (const auto &layer : toFind) {
                    std::cout << "\n\t- " << layer;
                }
                std::cout << std::endl;
                // REPORT_ERROR
                exit(1);
            }
        }

        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // Setup one-use debug listener if necessary

        // cppcheck-suppress unreadVariable; bug in cppcheck <2.9
        auto debugProbe = errorHandler->attachDebugProbe(createInfo);

        // Create instance

        handleVkResult("Could not create VkInstance",
                       vkCreateInstance(&createInfo, nullptr, &instance));
    }

    /*
     * Setup debug
     */
    errorHandler->onInstanceReady();

    /*
     * Create surface
     */
    surface = std::make_unique<Surface>(*this);

    /*
     * Pick physical device
     */
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            std::cout << "No GPUs with Vulkan support found" << std::endl;
            // REPORT_ERROR
            exit(1);
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        std::vector<PhysicalDeviceData> choices;

        for (const auto &device : devices) {
            PhysicalDeviceData data = {};
            data.device = device;

            vkGetPhysicalDeviceProperties(device, &data.properties);
            vkGetPhysicalDeviceFeatures(device, &data.features);

            choices.push_back(data);
        }

        const auto &result =
            pickPhysicalDevice(choices, *this, deviceExtensions);
        physicalDevice = result.device;
    }

    /*
     * Setup queues
     */

    queues = std::make_unique<Queues>(physicalDevice, *this);

    /*
     * Create logical device
     */
    {
        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

        // Specify queues

        // cppcheck-suppress unreadVariable; bug in cppcheck <2.9
        auto queueRequests = queues->requestCreation(createInfo);

        // Specify features

        VkPhysicalDeviceFeatures deviceFeatures{};
        createInfo.pEnabledFeatures = &deviceFeatures;

        // Specify device extensions

        createInfo.enabledExtensionCount =
            static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        // Provide a copy of instance validation layers

        createInfo.enabledLayerCount =
            static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();

        // Create logical device

        handleVkResult(
            "Could not create logical device",
            vkCreateDevice(physicalDevice, &createInfo, nullptr, &device));

        // Store queue handles

        queues->storeHandles(device);
    }

    /*
     * Create command pool
     */
    commandPool =
        std::make_unique<CommandPool>(*this, queues->getGraphicsQueue());

    /*
     * Create texture descriptor manager
     */

    textureDescriptors = std::make_unique<TextureDescriptors>(*this);

    /*
     * Initialize adapter
     */
    adapter = std::make_unique<Adapter>(*this);

    /*
     * Initialize swap chain
     */
    swapChain = std::make_unique<SwapChain>(*this);

    /*
     * Create render pass
     */
    renderPass = std::make_unique<RenderPass>(*this);

    /*
     * Create pipeline
     */
    pipeline = std::make_unique<Pipeline>(*this);

    /*
     * Create swap chain
     */
    swapChain->recreate();

    /*
     * Create frames
     */
    for (auto &container : frames) {
        container.emplace(*this);
    }
    currentFrame = 0;

    gint = std::make_unique<progressia::main::GraphicsInterface>(this);
}

Vulkan::~Vulkan() {
    gint.reset();
    frames.clear();
    swapChain.reset();
    pipeline.reset();
    renderPass.reset();
    adapter.reset();
    textureDescriptors.reset();
    commandPool.reset();
    vkDestroyDevice(device, nullptr);
    surface.reset();
    errorHandler.reset();
    vkDestroyInstance(instance, nullptr);
}

VkInstance Vulkan::getInstance() const { return instance; }

VkPhysicalDevice Vulkan::getPhysicalDevice() const { return physicalDevice; }

VkDevice Vulkan::getDevice() const { return device; }

Surface &Vulkan::getSurface() { return *surface; }

const Surface &Vulkan::getSurface() const { return *surface; }

Queues &Vulkan::getQueues() { return *queues; }

const Queues &Vulkan::getQueues() const { return *queues; }

CommandPool &Vulkan::getCommandPool() { return *commandPool; }

const CommandPool &Vulkan::getCommandPool() const { return *commandPool; }

RenderPass &Vulkan::getRenderPass() { return *renderPass; }

const RenderPass &Vulkan::getRenderPass() const { return *renderPass; }

Pipeline &Vulkan::getPipeline() { return *pipeline; }

const Pipeline &Vulkan::getPipeline() const { return *pipeline; }

SwapChain &Vulkan::getSwapChain() { return *swapChain; }

const SwapChain &Vulkan::getSwapChain() const { return *swapChain; }

TextureDescriptors &Vulkan::getTextureDescriptors() {
    return *textureDescriptors;
}

const TextureDescriptors &Vulkan::getTextureDescriptors() const {
    return *textureDescriptors;
}

Adapter &Vulkan::getAdapter() { return *adapter; }

const Adapter &Vulkan::getAdapter() const { return *adapter; }

progressia::main::GraphicsInterface &Vulkan::getGint() { return *gint; }

const progressia::main::GraphicsInterface &Vulkan::getGint() const {
    return *gint;
}

VkFormat Vulkan::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                     VkImageTiling tiling,
                                     VkFormatFeatureFlags features) {

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR &&
            (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
                   (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    std::cout << "Could not find a suitable format" << std::endl;
    // REPORT_ERROR
    exit(1);
}

uint32_t Vulkan::findMemoryType(uint32_t allowedByDevice,
                                VkMemoryPropertyFlags desiredProperties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (((1 << i) & allowedByDevice) == 0) {
            continue;
        }
        if ((memProperties.memoryTypes[i].propertyFlags & desiredProperties) !=
            desiredProperties) {
            continue;
        }

        return i;
    }

    std::cout << "Could not find suitable memory type" << std::endl;
    // REPORT_ERROR
    exit(1);
    return -1;
}

void Vulkan::handleVkResult(const char *errorMessage, VkResult result) {
    errorHandler->handleVkResult(errorMessage, result);
}

Frame *Vulkan::getCurrentFrame() {
    if (isRenderingFrame) {
        return &*frames.at(currentFrame);
    }
    return nullptr;
}

uint64_t Vulkan::getLastStartedFrame() { return lastStartedFrame; }

std::size_t Vulkan::getFrameInFlightIndex() { return currentFrame; }

bool Vulkan::startRender() {
    if (currentFrame >= MAX_FRAMES_IN_FLIGHT - 1) {
        currentFrame = 0;
    } else {
        currentFrame++;
    }

    bool shouldContinue = frames.at(currentFrame)->startRender();
    if (!shouldContinue) {
        return false;
    }

    isRenderingFrame = true;
    lastStartedFrame++;

    return true;
}

void Vulkan::endRender() {
    gint->flush();
    isRenderingFrame = false;
    frames.at(currentFrame)->endRender();
}

void Vulkan::waitIdle() {
    if (device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device);
    }
}

/*
 * VulkanErrorHandler
 */

VulkanErrorHandler::VulkanErrorHandler(Vulkan &vulkan) : vulkan(vulkan) {
    // do nothing
}

VulkanErrorHandler::~VulkanErrorHandler() {
#ifdef VULKAN_ERROR_CHECKING
    vulkan.callVoid("vkDestroyDebugUtilsMessengerEXT",
                    (VkDebugUtilsMessengerEXT)debugMessenger, nullptr);
#endif
}

#ifdef VULKAN_ERROR_CHECKING
namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageType,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

    if (messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
        return VK_FALSE;
    }

    [[maybe_unused]] auto &vk = *reinterpret_cast<const Vulkan *>(pUserData);

    const char *severityStr =
        messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
            ? "\x1B[1;91m\x1B[40mERROR\x1B[0m"
        : messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
            ? "\x1B[1;93m\x1B[40mWARNING\x1B[0m"
        : messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
            ? "info"
            : "verbose";

    const char *typeStr;
    switch (messageType) {
    case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
        typeStr = "general";
        break;
    case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
        typeStr = "violation";
        break;
    default:
        typeStr = "performance";
        break;
    }

    std::cout << "[Vulkan] [" << typeStr << " / " << severityStr << "]\t"
              << pCallbackData->pMessage << std::endl;
    // REPORT_ERROR
    return VK_FALSE;
}

void populateDebugMessengerCreateInfo(
    VkDebugUtilsMessengerCreateInfoEXT &createInfo, Vulkan &vulkan) {
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;

    createInfo.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = &vulkan;
}

} // namespace
#endif

std::unique_ptr<VkDebugUtilsMessengerCreateInfoEXT>
VulkanErrorHandler::attachDebugProbe(VkInstanceCreateInfo &createInfo) {
#ifdef VULKAN_ERROR_CHECKING

    std::unique_ptr result =
        std::make_unique<VkDebugUtilsMessengerCreateInfoEXT>();

    populateDebugMessengerCreateInfo(*result, vulkan);

    result->pNext = createInfo.pNext;
    createInfo.pNext = &*result;

    return result;

#else

    (void)createInfo;
    return std::unique_ptr<VkDebugUtilsMessengerCreateInfoEXT>();

#endif
}

void VulkanErrorHandler::onInstanceReady() {
#ifdef VULKAN_ERROR_CHECKING
    std::cout << "Registering debug callback" << std::endl;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    populateDebugMessengerCreateInfo(createInfo, vulkan);

    handleVkResult("Could not register debug messanger",
                   vulkan.call("vkCreateDebugUtilsMessengerEXT", &createInfo,
                               nullptr, &debugMessenger));

#endif
}

void VulkanErrorHandler::handleVkResult(const char *errorMessage,
                                        VkResult result) {
    if (result == VK_SUCCESS) {
        return;
    }

    std::cout << "Vulkan error (" << result << "): " << errorMessage
              << std::endl;
    // REPORT_ERROR
    exit(1);
}

/*
 * Surface
 */

Surface::Surface(Vulkan &vulkan) : vulkan(vulkan) {
    vulkan.handleVkResult("Could not create window surface (what?)",
                          glfwCreateWindowSurface(vulkan.getInstance(),
                                                  getGLFWWindowHandle(),
                                                  nullptr, &vk));
}

Surface::~Surface() { vkDestroySurfaceKHR(vulkan.getInstance(), vk, nullptr); }

VkSurfaceKHR Surface::getVk() { return vk; }

/*
 * Queue
 */

Queue::Queue(Test test) : test(test) {
    // do nothing
}

bool Queue::isSuitable(VkPhysicalDevice physicalDevice, uint32_t familyIndex,
                       Vulkan &vulkan,
                       const VkQueueFamilyProperties &properties) const {

    return test(physicalDevice, familyIndex, vulkan, properties);
}

VkQueue Queue::getVk() const { return vk; }

uint32_t Queue::getFamilyIndex() const { return *familyIndex; }

void Queue::waitIdle() const { vkQueueWaitIdle(vk); }

/*
 * Queues
 */

namespace {

bool graphicsQueueTest(VkPhysicalDevice, uint32_t, Vulkan &,
                       const VkQueueFamilyProperties &properties) {

    return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
}

bool presentQueueTest(VkPhysicalDevice physicalDevice, uint32_t familyIndex,
                      Vulkan &vulkan, const VkQueueFamilyProperties &) {

    VkBool32 presentSupport = false;

    vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, familyIndex,
                                         vulkan.getSurface().getVk(),
                                         &presentSupport);

    return presentSupport;
}

} // namespace

Queues::Queues(VkPhysicalDevice physicalDevice, Vulkan &vulkan)
    : graphicsQueue(graphicsQueueTest), presentQueue(presentQueueTest) {

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             nullptr);

    std::vector<VkQueueFamilyProperties> properties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount,
                                             properties.data());

    for (std::size_t index = 0; index < queueFamilyCount; index++) {

        for (auto queue : {&graphicsQueue, &presentQueue}) {
            if (!queue->isSuitable(physicalDevice, index, vulkan,
                                   properties[index])) {
                continue;
            }

            queue->familyIndex = index;
        }

        if (isComplete()) {
            break;
        }
    }
}

Queues::~Queues() {
    // do nothing
}

void Queues::storeHandles(VkDevice device) {
    for (auto queue : {&graphicsQueue, &presentQueue}) {
        vkGetDeviceQueue(device, queue->getFamilyIndex(), 0, &queue->vk);
    }
}

std::unique_ptr<Queues::CreationRequest>
Queues::requestCreation(VkDeviceCreateInfo &createInfo) const {

    std::unique_ptr result = std::make_unique<CreationRequest>();
    result->priority = 1.0f;

    std::unordered_set<uint32_t> uniqueQueues;
    for (const auto *queue : {&graphicsQueue, &presentQueue}) {
        uniqueQueues.insert(queue->getFamilyIndex());
    }

    for (const auto &index : uniqueQueues) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = index;
        queueCreateInfo.pQueuePriorities = &result->priority;
        queueCreateInfo.queueCount = 1;

        result->queueCreateInfos.push_back(queueCreateInfo);
    }

    createInfo.pQueueCreateInfos = result->queueCreateInfos.data();
    createInfo.queueCreateInfoCount =
        static_cast<uint32_t>(result->queueCreateInfos.size());

    return result;
}

bool Queues::isComplete() const {
    for (auto queue : {&graphicsQueue, &presentQueue}) {
        if (!queue->familyIndex.has_value()) {
            return false;
        }
    }

    return true;
}

const Queue &Queues::getGraphicsQueue() const { return graphicsQueue; }

const Queue &Queues::getPresentQueue() const { return presentQueue; }

/*
 * CommandPool
 */

CommandPool::CommandPool(Vulkan &vulkan, const Queue &queue)
    : queue(queue), vulkan(vulkan) {

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queue.getFamilyIndex();

    vulkan.handleVkResult(
        "Could not create CommandPool",
        vkCreateCommandPool(vulkan.getDevice(), &poolInfo, nullptr, &pool));
}

CommandPool::~CommandPool() {
    vkDestroyCommandPool(vulkan.getDevice(), pool, nullptr);
}

VkCommandBuffer CommandPool::allocateCommandBuffer() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vulkan.getDevice(), &allocInfo, &commandBuffer);

    return commandBuffer;
}

void CommandPool::beginCommandBuffer(VkCommandBuffer commandBuffer,
                                     VkCommandBufferUsageFlags usage) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usage;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);
}

VkCommandBuffer CommandPool::beginSingleUse() {
    VkCommandBuffer buffer = allocateCommandBuffer();
    beginCommandBuffer(buffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    return buffer;
}

void CommandPool::runSingleUse(VkCommandBuffer buffer, bool waitIdle) {
    vkEndCommandBuffer(buffer);
    submitMultiUse(buffer, false);

    if (waitIdle) {
        queue.waitIdle();
    }

    freeMultiUse(buffer);
}

VkCommandBuffer CommandPool::allocateMultiUse() {
    return allocateCommandBuffer();
}

VkCommandBuffer CommandPool::beginMultiUse() {
    VkCommandBuffer buffer = allocateMultiUse();
    beginCommandBuffer(buffer, 0);
    return buffer;
}

void CommandPool::submitMultiUse(VkCommandBuffer buffer, bool waitIdle) {
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    vkQueueSubmit(queue.getVk(), 1, &submitInfo, VK_NULL_HANDLE);

    if (waitIdle) {
        queue.waitIdle();
    }
}

void CommandPool::freeMultiUse(VkCommandBuffer buffer) {
    vkFreeCommandBuffers(vulkan.getDevice(), pool, 1, &buffer);
}

} // namespace desktop
} // namespace progressia

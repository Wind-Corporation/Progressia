#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_set>
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../../main/util.h"

#include "../../main/logging.h"
#include "../../main/rendering/graphics_interface.h"

namespace progressia::desktop {

namespace CstrUtils {
struct CstrHash {
    std::size_t operator()(const char *s) const noexcept {
        std::size_t acc = 0;

        while (*s != 0) {
            acc = acc * 31 + *s;
            s++;
        }

        return acc;
    }
};

struct CstrEqual {
    bool operator()(const char *lhs, const char *rhs) const noexcept {
        return strcmp(lhs, rhs) == 0;
    }
};

struct CstrCompare {
    bool operator()(const char *lhs, const char *rhs) const noexcept {
        return strcmp(lhs, rhs) < 0;
    }
};

using CstrHashSet = std::unordered_set<const char *, CstrHash, CstrEqual>;
} // namespace CstrUtils

class VkObjectWrapper : private progressia::main::NonCopyable {
    // empty
};

constexpr std::size_t MAX_FRAMES_IN_FLIGHT = 2;

class VulkanErrorHandler;
class PhysicalDevice;
class Surface;
class Queue;
class Queues;
class CommandPool;
class RenderPass;
class Pipeline;
class SwapChain;
class TextureDescriptors;
class Adapter;
class Frame;

class Vulkan : public VkObjectWrapper {
  private:
    VkInstance instance = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;

    std::unique_ptr<VulkanErrorHandler> errorHandler;
    std::unique_ptr<PhysicalDevice> physicalDevice;
    std::unique_ptr<Surface> surface;
    std::unique_ptr<Queues> queues;
    std::unique_ptr<CommandPool> commandPool;
    std::unique_ptr<RenderPass> renderPass;
    std::unique_ptr<Pipeline> pipeline;
    std::unique_ptr<SwapChain> swapChain;
    std::unique_ptr<TextureDescriptors> textureDescriptors;
    std::unique_ptr<Adapter> adapter;

    std::unique_ptr<progressia::main::GraphicsInterface> gint;

    std::vector<std::optional<Frame>> frames;
    std::size_t currentFrame;
    bool isRenderingFrame;
    uint64_t lastStartedFrame;

  public:
    Vulkan(std::vector<const char *> instanceExtensions,
           std::vector<const char *> deviceExtensions,
           std::vector<const char *> validationLayers);

    ~Vulkan();

    VkInstance getInstance() const;
    VkDevice getDevice() const;

    const PhysicalDevice &getPhysicalDevice() const;
    Surface &getSurface();
    const Surface &getSurface() const;
    Queues &getQueues();
    const Queues &getQueues() const;
    SwapChain &getSwapChain();
    const SwapChain &getSwapChain() const;
    CommandPool &getCommandPool();
    const CommandPool &getCommandPool() const;
    RenderPass &getRenderPass();
    const RenderPass &getRenderPass() const;
    Pipeline &getPipeline();
    const Pipeline &getPipeline() const;
    TextureDescriptors &getTextureDescriptors();
    const TextureDescriptors &getTextureDescriptors() const;
    Adapter &getAdapter();
    const Adapter &getAdapter() const;

    Frame *getCurrentFrame();
    const Frame *getCurrentFrame() const;

    progressia::main::GraphicsInterface &getGint();
    const progressia::main::GraphicsInterface &getGint() const;

    /*
     * Returns false when the frame should be skipped
     */
    bool startRender();
    void endRender();

    uint64_t getLastStartedFrame() const;
    std::size_t getFrameInFlightIndex() const;

    void waitIdle();

    VkFormat findSupportedFormat(const std::vector<VkFormat> &, VkImageTiling,
                                 VkFormatFeatureFlags);
    uint32_t findMemoryType(uint32_t allowedByDevice,
                            VkMemoryPropertyFlags desiredProperties);

    template <typename... Args>
    VkResult call(const char *functionName, Args &&...args) {

        using FunctionSignature = VkResult(VkInstance, Args...);

        auto func = reinterpret_cast<FunctionSignature *>(
            vkGetInstanceProcAddr(instance, functionName));
        if (func != nullptr) {
            return func(instance, std::forward<Args>(args)...);
        } else {
            progressia::main::logging::error()
                << "[Vulkan] [dynVkCall / VkResult]\tFunction not found for "
                   "name \""
                << functionName << "\"";
            // REPORT_ERROR
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    template <typename... Args>
    VkResult callVoid(const char *functionName, Args &&...args) {

        using FunctionSignature = void(VkInstance, Args...);

        auto func = reinterpret_cast<FunctionSignature *>(
            vkGetInstanceProcAddr(instance, functionName));
        if (func != nullptr) {
            func(instance, std::forward<Args>(args)...);
            return VK_SUCCESS;
        } else {
            progressia::main::logging::error()
                << "[Vulkan] [dynVkCall / void]\tFunction not found for name \""
                << functionName << "\"";
            // REPORT_ERROR
            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }
    }

    void handleVkResult(const char *errorMessage, VkResult);
};

class VulkanErrorHandler : public VkObjectWrapper {
  private:
    VkDebugUtilsMessengerEXT debugMessenger;
    Vulkan &vulkan;

  public:
    VulkanErrorHandler(Vulkan &vulkan);

    std::unique_ptr<VkDebugUtilsMessengerCreateInfoEXT>
    attachDebugProbe(VkInstanceCreateInfo &);
    void onInstanceReady();

    // NOLINTNEXTLINE(performance-trivially-destructible): fixing this makes code less readable due to use of macros in implementation
    ~VulkanErrorHandler();

    void handleVkResult(const char *errorMessage, VkResult result);
};

class Surface : public VkObjectWrapper {
  private:
    VkSurfaceKHR vk;
    Vulkan &vulkan;

  public:
    Surface(Vulkan &vulkan);
    ~Surface();

    VkSurfaceKHR getVk();
};

class Queue {
  private:
    using Test = std::function<bool(VkPhysicalDevice, uint32_t, Vulkan &,
                                    const VkQueueFamilyProperties &)>;

    Test test;
    std::optional<uint32_t> familyIndex;
    VkQueue vk;

    friend class Queues;

    Queue(Test test);

  public:
    bool isSuitable(VkPhysicalDevice, uint32_t familyIndex, Vulkan &,
                    const VkQueueFamilyProperties &) const;

    VkQueue getVk() const;
    uint32_t getFamilyIndex() const;

    void waitIdle() const;
};

class Queues {
  private:
    Queue graphicsQueue;
    Queue presentQueue;

  public:
    Queues(VkPhysicalDevice physicalDevice, Vulkan &vulkan);
    ~Queues();

    // cppcheck-suppress functionConst; this method modifies the Queue fields
    void storeHandles(VkDevice device);

    bool isComplete() const;

    struct CreationRequest {
        float priority;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    };
    std::unique_ptr<CreationRequest>
    requestCreation(VkDeviceCreateInfo &) const;

    const Queue &getGraphicsQueue() const;
    const Queue &getPresentQueue() const;
};

class CommandPool : public VkObjectWrapper {

  private:
    VkCommandPool pool;
    const Queue &queue;
    Vulkan &vulkan;

    VkCommandBuffer allocateCommandBuffer();
    void beginCommandBuffer(VkCommandBuffer commandBuffer,
                            VkCommandBufferUsageFlags usage);

  public:
    CommandPool(Vulkan &vulkan, const Queue &queue);
    ~CommandPool();

    VkCommandBuffer beginSingleUse();
    void runSingleUse(VkCommandBuffer, bool waitIdle = false);

    VkCommandBuffer allocateMultiUse();
    VkCommandBuffer beginMultiUse();
    void submitMultiUse(VkCommandBuffer, bool waitIdle = false);
    void freeMultiUse(VkCommandBuffer);
};

} // namespace progressia::desktop

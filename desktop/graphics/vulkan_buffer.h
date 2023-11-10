#pragma once

#include <vector>

#include "vulkan_common.h"

namespace progressia::desktop {

/*
 * A single buffer with a chunk of allocated memory.
 */
template <typename Item> class Buffer : public VkObjectWrapper {

  private:
    std::size_t itemCount;

  public:
    VkBuffer buffer;
    VkDeviceMemory memory;

    Vulkan &vulkan;

    Buffer(std::size_t itemCount, VkBufferUsageFlags usage,
           VkMemoryPropertyFlags properties, Vulkan &vulkan)
        :

          itemCount(itemCount), vulkan(vulkan) {

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = getSize();
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vulkan.handleVkResult(
            "Could not create a buffer",
            vkCreateBuffer(vulkan.getDevice(), &bufferInfo, nullptr, &buffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vulkan.getDevice(), buffer,
                                      &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            vulkan.findMemoryType(memRequirements.memoryTypeBits, properties);

        vulkan.handleVkResult(
            "Could not allocate memory for a buffer",
            vkAllocateMemory(vulkan.getDevice(), &allocInfo, nullptr, &memory));

        vkBindBufferMemory(vulkan.getDevice(), buffer, memory, 0);
    }

    ~Buffer() {
        if (buffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(vulkan.getDevice(), buffer, nullptr);
        }

        if (memory != VK_NULL_HANDLE) {
            vkFreeMemory(vulkan.getDevice(), memory, nullptr);
        }
    }

    std::size_t getItemCount() const { return itemCount; }

    std::size_t getSize() const { return sizeof(Item) * itemCount; }

    void *map() {
        void *dst;
        vkMapMemory(vulkan.getDevice(), memory, 0, getSize(), 0, &dst);
        return dst;
    }

    void unmap() { vkUnmapMemory(vulkan.getDevice(), memory); }
};

/*
 * A buffer that is optimized for reading by the device. This buffer uses a
 * secondary staging buffer.
 */
template <typename Item> class FastReadBuffer : public VkObjectWrapper {
  private:
    VkCommandBuffer commandBuffer;
    Vulkan &vulkan;

  public:
    Buffer<Item> stagingBuffer;
    Buffer<Item> remoteBuffer;

    FastReadBuffer(std::size_t itemCount, VkBufferUsageFlags usage,
                   Vulkan &vulkan)
        :

          vulkan(vulkan),

          stagingBuffer(itemCount, usage | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                        vulkan),

          remoteBuffer(itemCount, usage | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                       VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vulkan) {
        recordCopyCommands();
    }

    ~FastReadBuffer() { vulkan.getCommandPool().freeMultiUse(commandBuffer); }

  private:
    void recordCopyCommands() {
        commandBuffer = vulkan.getCommandPool().beginMultiUse();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = getSize();
        vkCmdCopyBuffer(commandBuffer, stagingBuffer.buffer,
                        remoteBuffer.buffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);
    }

  public:
    void flush() const {
        vulkan.getCommandPool().submitMultiUse(commandBuffer, true);
    }

    void load(const Item *data) const {
        void *dst;
        vkMapMemory(vulkan.getDevice(), stagingBuffer.memory, 0, getSize(), 0,
                    &dst);
        memcpy(dst, data, getSize());
        vkUnmapMemory(vulkan.getDevice(), stagingBuffer.memory);

        flush();
    }

    std::size_t getItemCount() const { return stagingBuffer.getItemCount(); }

    std::size_t getSize() const { return stagingBuffer.getSize(); }

    Vulkan &getVulkan() { return vulkan; }

    const Vulkan &getVulkan() const { return vulkan; }
};

/*
 * A pair of a vertex buffer and an index buffer.
 */
template <typename Vertex, typename Index, VkIndexType INDEX_TYPE>
class IndexedBufferBase : public VkObjectWrapper {

  private:
    FastReadBuffer<Vertex> vertexBuffer;
    FastReadBuffer<Index> indexBuffer;

  public:
    IndexedBufferBase(std::size_t vertexCount, std::size_t indexCount,
                      Vulkan &vulkan)
        :

          vertexBuffer(vertexCount, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, vulkan),
          indexBuffer(indexCount, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, vulkan) {

        // Do nothing
    }

    void load(const Vertex *vertices, const Index *indices) const {
        vertexBuffer.load(vertices);
        indexBuffer.load(indices);
    }

    void draw(VkCommandBuffer commandBuffer) {
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1,
                               &vertexBuffer.remoteBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer.remoteBuffer.buffer, 0,
                             INDEX_TYPE);
        vkCmdDrawIndexed(commandBuffer,
                         static_cast<uint32_t>(indexBuffer.getItemCount()), 1,
                         0, 0, 0);
    }

    Vulkan &getVulkan() { return vertexBuffer.getVulkan(); }

    const Vulkan &getVulkan() const { return vertexBuffer.getVulkan(); }
};

template <typename Vertex>
using IndexedBuffer = IndexedBufferBase<Vertex, uint16_t, VK_INDEX_TYPE_UINT16>;

} // namespace progressia::desktop

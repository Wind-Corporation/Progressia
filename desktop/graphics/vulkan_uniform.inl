#pragma once

#include <cstring>

#include "../../main/util.h"
#include "vulkan_frame.h"
#include "vulkan_pipeline.h"

namespace progressia {
namespace desktop {

template <typename... Entries>
Uniform<Entries...>::StateImpl::Set::Set(VkDescriptorSet vk, Vulkan &vulkan)
    : vk(vk),
      contents((sizeof(Entries) + ...), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               vulkan) {}

template <typename... Entries>
Uniform<Entries...>::StateImpl::StateImpl(
    const std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> &vks,
    Vulkan &vulkan)
    : setsToUpdate(0) {
    constexpr std::size_t COUNT = sizeof...(Entries) * MAX_FRAMES_IN_FLIGHT;

    std::array<VkDescriptorBufferInfo, COUNT> bufferInfos;
    std::array<VkWriteDescriptorSet, COUNT> writes;
    std::size_t index = 0;

    for (std::size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        auto &set = sets.at(i);
        set.emplace(vks.at(i), vulkan);

        std::size_t offset = 0;
        FOR_PACK_S(Entries, Entry, {
            bufferInfos[index] = {};
            bufferInfos[index].buffer = set->contents.buffer;
            bufferInfos[index].offset = offset;
            bufferInfos[index].range = sizeof(Entry);

            writes[index] = {};
            writes[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[index].dstSet = set->vk;
            writes[index].dstBinding = index % sizeof...(Entries);
            writes[index].dstArrayElement = 0;
            writes[index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            writes[index].descriptorCount = 1;
            writes[index].pBufferInfo = &bufferInfos[index];

            offset += sizeof(Entry);
            index++;
        })
    }

    vkUpdateDescriptorSets(vulkan.getDevice(), writes.size(), writes.data(), 0,
                           nullptr);
}

template <typename... Entries>
Uniform<Entries...>::State::State(std::size_t id, Uniform *uniform)
    : id(id), uniform(uniform) {}

template <typename... Entries>
Uniform<Entries...>::State::State() : id(-1), uniform(nullptr) {}

template <typename... Entries>
void Uniform<Entries...>::State::update(const Entries &...entries) {
    auto &state = *uniform->states.at(id);

    auto *dst = state.newContents.data();
    FOR_PACK(Entries, entries, e, {
        std::memcpy(dst, &e, sizeof(e));
        dst += sizeof(e);
    })
    state.setsToUpdate = state.sets.size();
}

template <typename... Entries> void Uniform<Entries...>::State::bind() {
    auto &state = *uniform->states.at(id);
    auto &set = *state.sets.at(uniform->vulkan.getFrameInFlightIndex());

    // REPORT_ERROR if getCurrentFrame() == nullptr
    auto commandBuffer = uniform->vulkan.getCurrentFrame()->getCommandBuffer();
    auto pipelineLayout = uniform->vulkan.getPipeline().getLayout();

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout, uniform->getSetNumber(), 1, &set.vk,
                            0, nullptr);
}

template <typename... Entries>
Uniform<Entries...>::Uniform(uint32_t setNumber, Vulkan &vulkan)
    : DescriptorSetInterface(setNumber, vulkan) {
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

    std::array<VkDescriptorSetLayoutBinding, sizeof...(Entries)> bindings;
    for (std::size_t i = 0; i < bindings.size(); i++) {
        bindings[i] = {};
        bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[i].descriptorCount = 1;
        bindings[i].stageFlags = VK_SHADER_STAGE_VERTEX_BIT |
                                 VK_SHADER_STAGE_FRAGMENT_BIT; // TODO optimize?
        bindings[i].pImmutableSamplers = nullptr;
        bindings[i].binding = i;
    }

    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    vulkan.handleVkResult("Could not create uniform descriptor set layout",
                          vkCreateDescriptorSetLayout(vulkan.getDevice(),
                                                      &layoutInfo, nullptr,
                                                      &layout));

    allocatePool();
}

template <typename... Entries> Uniform<Entries...>::~Uniform() {
    for (auto pool : pools) {
        vkDestroyDescriptorPool(vulkan.getDevice(), pool, nullptr);
    }

    vkDestroyDescriptorSetLayout(vulkan.getDevice(), layout, nullptr);
}

template <typename... Entries> void Uniform<Entries...>::allocatePool() {
    pools.resize(pools.size() + 1);

    std::array<VkDescriptorPoolSize, 1> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = sizeof...(Entries) * POOL_SIZE;

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = POOL_SIZE;

    auto output = &pools[pools.size() - 1];
    vulkan.handleVkResult(
        "Could not create uniform descriptor pool",
        vkCreateDescriptorPool(vulkan.getDevice(), &poolInfo, nullptr, output));

    lastPoolCapacity = POOL_SIZE;
}

template <typename... Entries>
typename Uniform<Entries...>::State Uniform<Entries...>::addState() {
    if (lastPoolCapacity < MAX_FRAMES_IN_FLIGHT) {
        allocatePool();
    }

    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> vks;

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = pools.back();
    allocInfo.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;

    std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts;
    layouts.fill(layout);

    allocInfo.pSetLayouts = layouts.data();

    vulkan.handleVkResult(
        "Could not create descriptor set",
        vkAllocateDescriptorSets(vulkan.getDevice(), &allocInfo, vks.data()));

    lastPoolCapacity -= MAX_FRAMES_IN_FLIGHT;

    states.push_back(std::make_unique<StateImpl>(vks, vulkan));

    return State(states.size() - 1, this);
}

template <typename... Entries> void Uniform<Entries...>::doUpdates() {
    for (auto &state : states) {
        auto &buffer = state->sets.at(vulkan.getFrameInFlightIndex())->contents;
        auto &src = state->newContents;

        if (state->setsToUpdate > 0) {
            auto *dst = buffer.map();
            std::memcpy(dst, src.data(), src.size());
            buffer.unmap();

            state->setsToUpdate--;
        }
    }
}

} // namespace desktop
} // namespace progressia

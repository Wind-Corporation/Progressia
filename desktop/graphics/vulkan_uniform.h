#pragma once

#include <array>
#include <memory>
#include <vector>

#include "vulkan_buffer.h"
#include "vulkan_common.h"
#include "vulkan_descriptor_set.h"

namespace progressia {
namespace desktop {

template <typename... Entries> class Uniform : public DescriptorSetInterface {

  private:
    constexpr static uint32_t POOL_SIZE = 64;

    std::vector<VkDescriptorPool> pools;

    struct StateImpl {
        struct Set {
            VkDescriptorSet vk;
            Buffer<unsigned char> contents;

            Set(VkDescriptorSet, Vulkan &);
        };

        std::array<std::optional<Set>, MAX_FRAMES_IN_FLIGHT> sets;

        std::array<unsigned char, (sizeof(Entries) + ...)> newContents;
        uint64_t setsToUpdate;

        StateImpl(const std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> &vks,
                  Vulkan &);
    };

    std::vector<std::unique_ptr<StateImpl>> states;

    uint32_t lastPoolCapacity;

    void allocatePool();

  public:
    class State {

      private:
        std::size_t id;

      public:
        Uniform<Entries...> *uniform;

      private:
        friend class Uniform<Entries...>;
        State(std::size_t id, Uniform<Entries...> *);

        void doUpdate();

      public:
        State();

        void update(const Entries &...entries);
        void bind();
    };

    Uniform(uint32_t setNumber, Vulkan &);
    ~Uniform();

    State addState();

    void doUpdates();
};

} // namespace desktop
} // namespace progressia

#include "vulkan_uniform.inl"

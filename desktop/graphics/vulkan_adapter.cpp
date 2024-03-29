#include "vulkan_adapter.h"

#include "vulkan_common.h"

#include <array>
#include <cstddef>
#include <fstream>
#include <memory>
#include <type_traits>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/euler_angles.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "../../main/logging.h"
#include "../../main/rendering.h"
#include "vulkan_buffer.h"
#include "vulkan_frame.h"
#include "vulkan_pipeline.h"
#include "vulkan_swap_chain.h"
#include "vulkan_texture_descriptors.h"

#include <embedded_resources.h>

namespace progressia::desktop {

using progressia::main::Vertex;

namespace {

struct FieldProperties {
    uint32_t offset;
    VkFormat format;
};

auto getVertexFieldProperties() {
    return std::array{
        FieldProperties{offsetof(Vertex, position), VK_FORMAT_R32G32B32_SFLOAT},
        FieldProperties{offsetof(Vertex, color), VK_FORMAT_R32G32B32A32_SFLOAT},
        FieldProperties{offsetof(Vertex, normal), VK_FORMAT_R32G32B32_SFLOAT},
        FieldProperties{offsetof(Vertex, texCoord), VK_FORMAT_R32G32_SFLOAT},
    };
}

} // namespace

namespace {
std::vector<char> tmp_readFile(const std::string &path) {
    auto resource = __embedded_resources::getEmbeddedResource(path);

    if (resource.data == nullptr) {
        // REPORT_ERROR
        progressia::main::logging::fatal()
            << "Could not find resource \"" << path << "\"";
        exit(1);
    }

    return {resource.data, resource.data + resource.length};
}
} // namespace

Adapter::Adapter(Vulkan &vulkan)
    : vulkan(vulkan), viewUniform(0, vulkan), lightUniform(2, vulkan) {

    attachments.push_back(
        {"Depth buffer",

         vulkan.findSupportedFormat(
             {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
              VK_FORMAT_D24_UNORM_S8_UINT},
             VK_IMAGE_TILING_OPTIMAL,
             VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT),

         VK_IMAGE_ASPECT_DEPTH_BIT,
         VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
         VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
         VK_ATTACHMENT_LOAD_OP_CLEAR,
         VK_ATTACHMENT_STORE_OP_DONT_CARE,

         {1.0F, 0},

         nullptr});
}

Adapter::~Adapter() = default;

std::vector<Attachment> &Adapter::getAttachments() { return attachments; }

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): future-proofing
std::vector<char> Adapter::loadVertexShader() {
    return tmp_readFile("shader.vert.spv");
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): future-proofing
std::vector<char> Adapter::loadFragmentShader() {
    return tmp_readFile("shader.frag.spv");
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): future-proofing
VkVertexInputBindingDescription Adapter::getVertexInputBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription>
// NOLINTNEXTLINE(readability-convert-member-functions-to-static): future-proofing
Adapter::getVertexInputAttributeDescriptions() {
    std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

    uint32_t i = 0;
    for (auto props : getVertexFieldProperties()) {
        attributeDescriptions.push_back({});

        attributeDescriptions[i].binding = 0;
        attributeDescriptions[i].location = i;
        attributeDescriptions[i].format = props.format;
        attributeDescriptions[i].offset = props.offset;

        i++;
    }

    return attributeDescriptions;
}

std::vector<VkDescriptorSetLayout> Adapter::getUsedDSLayouts() const {
    return {viewUniform.getLayout(), vulkan.getTextureDescriptors().getLayout(),
            lightUniform.getLayout()};
}

Adapter::ViewUniform::State Adapter::createView() {
    return viewUniform.addState();
}

Adapter::LightUniform::State Adapter::createLight() {
    return lightUniform.addState();
}

void Adapter::onPreFrame() {
    viewUniform.doUpdates();
    lightUniform.doUpdates();
}

/*
 * graphics_interface implementation
 */

} // namespace progressia::desktop
namespace progressia::main {

using namespace progressia::desktop;

namespace {
struct DrawRequest {
    progressia::desktop::Texture *texture;
    IndexedBuffer<Vertex> *vertices;
    glm::mat4 modelTransform;
};

// NOLINTNEXTLINE: TODO
std::vector<DrawRequest> pendingDrawCommands;
constexpr std::size_t PENDING_DRAW_COMMANDS_MAX_SIZE = 100000;

// NOLINTNEXTLINE: TODO
glm::mat4 currentModelTransform;

} // namespace

struct progressia::main::Texture::Backend {
    progressia::desktop::Texture texture;
};

progressia::main::Texture::Texture(std::unique_ptr<Backend> backend)
    : backend(std::move(backend)) {}

progressia::main::Texture::~Texture() = default;

struct Primitive::Backend {
    IndexedBuffer<Vertex> buf;
    progressia::main::Texture *tex;
};

Primitive::Primitive(std::unique_ptr<Backend> backend)
    : backend(std::move(backend)) {}

Primitive::~Primitive() = default;

void Primitive::draw() {
    if (pendingDrawCommands.size() > PENDING_DRAW_COMMANDS_MAX_SIZE) {
        backend->buf.getVulkan().getGint().flush();
    }

    pendingDrawCommands.push_back({&backend->tex->backend->texture,
                                   &backend->buf, currentModelTransform});
}

const progressia::main::Texture *Primitive::getTexture() const {
    return backend->tex;
}

struct View::Backend {
    Adapter::ViewUniform::State state;
};

View::View(std::unique_ptr<Backend> backend) : backend(std::move(backend)) {}

View::~View() = default;

void View::configure(const glm::mat4 &proj, const glm::mat4 &view) {
    backend->state.update(proj, view);
}

void View::use() {
    backend->state.uniform->getVulkan().getGint().flush();
    backend->state.bind();
}

struct Light::Backend {
    Adapter::LightUniform::State state;
};

Light::Light(std::unique_ptr<Backend> backend) : backend(std::move(backend)) {}
Light::~Light() = default;

void Light::configure(const glm::vec3 &color, const glm::vec3 &from,
                      float contrast, float softness) {

    backend->state.update(Adapter::Light{glm::vec4(color, 1.0F),
                                         glm::vec4(glm::normalize(from), 1.0F),
                                         contrast, softness});
}

void Light::use() {
    backend->state.uniform->getVulkan().getGint().flush();
    backend->state.bind();
}

GraphicsInterface::GraphicsInterface(Backend backend) : backend(backend) {}

GraphicsInterface::~GraphicsInterface() = default;

std::unique_ptr<progressia::main::Texture>
GraphicsInterface::newTexture(const progressia::main::Image &src) {
    using Backend = progressia::main::Texture::Backend;

    return std::make_unique<progressia::main::Texture>(
        std::unique_ptr<Backend>(new Backend{progressia::desktop::Texture(
            src, *static_cast<Vulkan *>(this->backend))}));
}

std::unique_ptr<Primitive>
GraphicsInterface::newPrimitive(const std::vector<Vertex> &vertices,
                                const std::vector<Vertex::Index> &indices,
                                progressia::main::Texture *texture) {

    auto primitive = std::make_unique<Primitive>(
        std::unique_ptr<Primitive::Backend>(new Primitive::Backend{
            IndexedBuffer<Vertex>(vertices.size(), indices.size(),
                                  *static_cast<Vulkan *>(this->backend)),
            texture}));

    primitive->backend->buf.load(vertices.data(), indices.data());

    return primitive;
}

std::unique_ptr<View> GraphicsInterface::newView() {
    return std::make_unique<View>(std::unique_ptr<View::Backend>(
        new View::Backend{Adapter::ViewUniform::State(
            static_cast<Vulkan *>(this->backend)->getAdapter().createView())}));
}

std::unique_ptr<Light> GraphicsInterface::newLight() {
    return std::make_unique<Light>(
        std::unique_ptr<Light::Backend>(new Light::Backend{
            Adapter::LightUniform::State(static_cast<Vulkan *>(this->backend)
                                             ->getAdapter()
                                             .createLight())}));
}

glm::vec2 GraphicsInterface::getViewport() const {
    auto extent =
        static_cast<const Vulkan *>(this->backend)->getSwapChain().getExtent();
    return {extent.width, extent.height};
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static): future-proofing
void GraphicsInterface::setModelTransform(const glm::mat4 &m) {
    currentModelTransform = m;
}

void GraphicsInterface::flush() {

    auto *commandBuffer = static_cast<Vulkan *>(this->backend)
                              ->getCurrentFrame()
                              ->getCommandBuffer();
    auto *pipelineLayout =
        static_cast<Vulkan *>(this->backend)->getPipeline().getLayout();

    progressia::desktop::Texture *lastTexture = nullptr;

    for (auto &cmd : pendingDrawCommands) {
        if (cmd.texture != lastTexture) {
            lastTexture = cmd.texture;
            cmd.texture->bind();
        }

        auto &m = cmd.modelTransform;
        // Evil transposition: column_major -> row_major
        // clang-format off
        std::remove_reference_t<decltype(m)>::value_type src[3*4] {
            m[0][0], m[0][1], m[0][2], m[0][3],
            m[1][0], m[1][1], m[1][2], m[1][3],
            m[2][0], m[2][1], m[2][2], m[2][3]
        };
        // clang-format on

        vkCmdPushConstants(
            // REPORT_ERROR if getCurrentFrame() == nullptr
            commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
            sizeof(src), &src);

        cmd.vertices->draw(commandBuffer);
    }

    pendingDrawCommands.clear();
}

// NOLINTNEXTLINE: TODO
float GraphicsInterface::tmp_getTime() { return glfwGetTime(); }

uint64_t GraphicsInterface::getLastStartedFrame() {
    return static_cast<Vulkan *>(this->backend)->getLastStartedFrame();
}

} // namespace progressia::main

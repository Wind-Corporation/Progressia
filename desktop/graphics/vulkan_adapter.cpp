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

namespace progressia {
namespace desktop {

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
    auto resource = __embedded_resources::getEmbeddedResource(path.c_str());

    if (resource.data == nullptr) {
        // REPORT_ERROR
        progressia::main::logging::fatal()
            << "Could not find resource \"" << path << "\"";
        exit(1);
    }

    return std::vector<char>(resource.data, resource.data + resource.length);
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

         {1.0f, 0},

         nullptr});
}

Adapter::~Adapter() {
    // Do nothing
}

std::vector<Attachment> &Adapter::getAttachments() { return attachments; }

std::vector<char> Adapter::loadVertexShader() {
    return tmp_readFile("shader.vert.spv");
}

std::vector<char> Adapter::loadFragmentShader() {
    return tmp_readFile("shader.frag.spv");
}

VkVertexInputBindingDescription Adapter::getVertexInputBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
}

std::vector<VkVertexInputAttributeDescription>
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

} // namespace desktop
namespace main {

using namespace progressia::desktop;

namespace {
struct DrawRequest {
    progressia::desktop::Texture *texture;
    IndexedBuffer<Vertex> *vertices;
    glm::mat4 modelTransform;
};

std::vector<DrawRequest> pendingDrawCommands;
glm::mat4 currentModelTransform;
} // namespace

progressia::main::Texture::Texture(Backend backend) : backend(backend) {}

progressia::main::Texture::~Texture() {
    delete static_cast<progressia::desktop::Texture *>(this->backend);
}

namespace {
struct PrimitiveBackend {

    IndexedBuffer<Vertex> buf;
    progressia::main::Texture *tex;
};
} // namespace

Primitive::Primitive(Backend backend) : backend(backend) {}

Primitive::~Primitive() {
    delete static_cast<PrimitiveBackend *>(this->backend);
}

void Primitive::draw() {
    auto backend = static_cast<PrimitiveBackend *>(this->backend);

    if (pendingDrawCommands.size() > 100000) {
        backend->buf.getVulkan().getGint().flush();
    }

    pendingDrawCommands.push_back(
        {static_cast<progressia::desktop::Texture *>(backend->tex->backend),
         &backend->buf, currentModelTransform});
}

const progressia::main::Texture *Primitive::getTexture() const {
    return static_cast<PrimitiveBackend *>(this->backend)->tex;
}

View::View(Backend backend) : backend(backend) {}

View::~View() {
    delete static_cast<Adapter::ViewUniform::State *>(this->backend);
}

void View::configure(const glm::mat4 &proj, const glm::mat4 &view) {

    static_cast<Adapter::ViewUniform::State *>(this->backend)
        ->update(proj, view);
}

void View::use() {
    auto backend = static_cast<Adapter::ViewUniform::State *>(this->backend);
    backend->uniform->getVulkan().getGint().flush();
    backend->bind();
}

Light::Light(Backend backend) : backend(backend) {}

Light::~Light() {
    delete static_cast<Adapter::LightUniform::State *>(this->backend);
}

void Light::configure(const glm::vec3 &color, const glm::vec3 &from,
                      float contrast, float softness) {

    static_cast<Adapter::LightUniform::State *>(this->backend)
        ->update(Adapter::Light{glm::vec4(color, 1.0f),
                                glm::vec4(glm::normalize(from), 1.0f), contrast,
                                softness});
}

void Light::use() {
    auto backend = static_cast<Adapter::LightUniform::State *>(this->backend);
    backend->uniform->getVulkan().getGint().flush();
    backend->bind();
}

GraphicsInterface::GraphicsInterface(Backend backend) : backend(backend) {}

GraphicsInterface::~GraphicsInterface() {
    // Do nothing
}

progressia::main::Texture *
GraphicsInterface::newTexture(const progressia::main::Image &src) {
    auto backend = new progressia::desktop::Texture(
        src, *static_cast<Vulkan *>(this->backend));

    return new Texture(backend);
}

Primitive *
GraphicsInterface::newPrimitive(const std::vector<Vertex> &vertices,
                                const std::vector<Vertex::Index> &indices,
                                progressia::main::Texture *texture) {

    auto backend = new PrimitiveBackend{
        IndexedBuffer<Vertex>(vertices.size(), indices.size(),
                              *static_cast<Vulkan *>(this->backend)),
        texture};

    backend->buf.load(vertices.data(), indices.data());

    return new Primitive(backend);
}

View *GraphicsInterface::newView() {
    return new View(new Adapter::ViewUniform::State(
        static_cast<Vulkan *>(this->backend)->getAdapter().createView()));
}

Light *GraphicsInterface::newLight() {
    return new Light(new Adapter::LightUniform::State(
        static_cast<Vulkan *>(this->backend)->getAdapter().createLight()));
}

glm::vec2 GraphicsInterface::getViewport() const {
    auto extent =
        static_cast<const Vulkan *>(this->backend)->getSwapChain().getExtent();
    return {extent.width, extent.height};
}

void GraphicsInterface::setModelTransform(const glm::mat4 &m) {
    currentModelTransform = m;
}

void GraphicsInterface::flush() {

    auto commandBuffer = static_cast<Vulkan *>(this->backend)
                             ->getCurrentFrame()
                             ->getCommandBuffer();
    auto pipelineLayout =
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

float GraphicsInterface::tmp_getTime() { return glfwGetTime(); }

uint64_t GraphicsInterface::getLastStartedFrame() {
    return static_cast<Vulkan *>(this->backend)->getLastStartedFrame();
}

} // namespace main
} // namespace progressia

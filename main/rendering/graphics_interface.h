#pragma once

#include "../util.h"
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "image.h"

namespace progressia::main {

struct Vertex {

    using Index = uint16_t;

    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Texture : private progressia::main::NonCopyable {
  private:
    struct Backend;
    std::unique_ptr<Backend> backend;
    friend class GraphicsInterface;

    friend class Primitive;

  public:
    Texture(std::unique_ptr<Backend>);
    ~Texture();
};

class Primitive : private progressia::main::NonCopyable {
  private:
    struct Backend;
    std::unique_ptr<Backend> backend;
    friend class GraphicsInterface;

  public:
    Primitive(std::unique_ptr<Backend>);
    ~Primitive();

    void draw();

    const Texture *getTexture() const;
};

class View : private progressia::main::NonCopyable {
  private:
    struct Backend;
    std::unique_ptr<Backend> backend;
    friend class GraphicsInterface;

  public:
    View(std::unique_ptr<Backend>);
    ~View();

    void configure(const glm::mat4 &proj, const glm::mat4 &view);
    void use();
};

class Light : private progressia::main::NonCopyable {
  private:
    struct Backend;
    std::unique_ptr<Backend> backend;
    friend class GraphicsInterface;

  public:
    Light(std::unique_ptr<Backend>);
    ~Light();

    void configure(const glm::vec3 &color, const glm::vec3 &from,
                   float contrast, float softness);
    void use();
};

class GraphicsInterface : private progressia::main::NonCopyable {
  public:
    using Backend = void *;

  private:
    Backend backend;

  public:
    GraphicsInterface(Backend);
    ~GraphicsInterface();

    std::unique_ptr<Texture> newTexture(const Image &);

    std::unique_ptr<Primitive> newPrimitive(const std::vector<Vertex> &,
                                            const std::vector<Vertex::Index> &,
                                            Texture *texture);

    glm::vec2 getViewport() const;

    void setModelTransform(const glm::mat4 &);

    std::unique_ptr<View> newView();
    std::unique_ptr<Light> newLight();

    void flush();
    void startNextLayer();

    float tmp_getTime();
    uint64_t getLastStartedFrame();
};

} // namespace progressia::main

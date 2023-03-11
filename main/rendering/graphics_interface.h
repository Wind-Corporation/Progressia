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

namespace progressia {
namespace main {

struct Vertex {

    using Index = uint16_t;

    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal;
    glm::vec2 texCoord;
};

class Texture : private progressia::main::NonCopyable {
  public:
    using Backend = void *;

  private:
    Backend backend;

    friend class Primitive;

  public:
    Texture(Backend);
    ~Texture();
};

class Primitive : private progressia::main::NonCopyable {
  public:
    using Backend = void *;

  private:
    Backend backend;

    friend class GraphicsInterface;

  public:
    Primitive(Backend);
    ~Primitive();

    void draw();

    const Texture *getTexture() const;
};

class View : private progressia::main::NonCopyable {
  public:
    using Backend = void *;

  private:
    Backend backend;

  public:
    View(Backend);
    ~View();

    void configure(const glm::mat4 &proj, const glm::mat4 &view);
    void use();
};

class Light : private progressia::main::NonCopyable {
  public:
    using Backend = void *;

  private:
    Backend backend;

  public:
    Light(Backend);
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

    Texture *newTexture(const Image &);

    Primitive *newPrimitive(const std::vector<Vertex> &,
                            const std::vector<Vertex::Index> &,
                            Texture *texture);

    glm::vec2 getViewport() const;

    void setModelTransform(const glm::mat4 &);

    View *newView();
    Light *newLight();

    void flush();
    void startNextLayer();

    float tmp_getTime();
    uint64_t getLastStartedFrame();
};

} // namespace main
} // namespace progressia

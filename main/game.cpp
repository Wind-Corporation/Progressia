#include "game.h"

#include <iostream>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtx/euler_angles.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "rendering.h"

#include "logging.h"
using namespace progressia::main::logging;

namespace progressia {
namespace main {

std::unique_ptr<Primitive> cube1, cube2;
std::unique_ptr<Texture> texture1, texture2;
std::unique_ptr<View> perspective;
std::unique_ptr<Light> light;

GraphicsInterface *gint;

void addRect(glm::vec3 origin, glm::vec3 width, glm::vec3 height,
             glm::vec4 color, std::vector<Vertex> &vertices,
             std::vector<Vertex::Index> &indices) {

    Vertex::Index offset = vertices.size();

    vertices.push_back({origin, color, {}, {0, 0}});
    vertices.push_back({origin + width, color, {}, {0, 1}});
    vertices.push_back({origin + width + height, color, {}, {1, 1}});
    vertices.push_back({origin + height, color, {}, {1, 0}});

    indices.push_back(offset + 0);
    indices.push_back(offset + 1);
    indices.push_back(offset + 2);

    indices.push_back(offset + 0);
    indices.push_back(offset + 2);
    indices.push_back(offset + 3);
}

void addBox(glm::vec3 origin, glm::vec3 length, glm::vec3 height,
            glm::vec3 depth, std::array<glm::vec4, 6> colors,
            std::vector<Vertex> &vertices,
            std::vector<Vertex::Index> &indices) {
    addRect(origin, height, length, colors[0], vertices, indices);
    addRect(origin, length, depth, colors[1], vertices, indices);
    addRect(origin, depth, height, colors[2], vertices, indices);
    addRect(origin + height, depth, length, colors[3], vertices, indices);
    addRect(origin + length, height, depth, colors[4], vertices, indices);
    addRect(origin + depth, length, height, colors[5], vertices, indices);
}

void initialize(GraphicsInterface &gintp) {

    debug("game init begin");
    gint = &gintp;

    texture1.reset(gint->newTexture(
        progressia::main::loadImage(u"../assets/texture.png")));
    texture2.reset(gint->newTexture(
        progressia::main::loadImage(u"../assets/texture2.png")));

    // Cube 1
    {
        std::vector<Vertex> vertices;
        std::vector<Vertex::Index> indices;
        auto white = glm::vec4(1, 1, 1, 1);

        addBox({-0.5, -0.5, -0.5}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
               {white, white, white, white, white, white}, vertices, indices);

        for (std::size_t i = 0; i < indices.size(); i += 3) {
            Vertex &a = vertices[indices[i + 0]];
            Vertex &b = vertices[indices[i + 1]];
            Vertex &c = vertices[indices[i + 2]];

            glm::vec3 x = b.position - a.position;
            glm::vec3 y = c.position - a.position;

            glm::vec3 normal = glm::normalize(glm::cross(x, y));

            a.normal = normal;
            b.normal = normal;
            c.normal = normal;
        }

        cube1.reset(gint->newPrimitive(vertices, indices, &*texture1));
    }

    // Cube 2
    {
        std::vector<Vertex> vertices;
        std::vector<Vertex::Index> indices;
        auto white = glm::vec4(1, 1, 1, 1);

        addBox({-0.5, -2.5, -0.5}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
               {white, white, white, white, white, white}, vertices, indices);

        for (std::size_t i = 0; i < indices.size(); i += 3) {
            Vertex &a = vertices[indices[i + 0]];
            Vertex &b = vertices[indices[i + 1]];
            Vertex &c = vertices[indices[i + 2]];

            glm::vec3 x = b.position - a.position;
            glm::vec3 y = c.position - a.position;

            glm::vec3 normal = glm::normalize(glm::cross(x, y));

            a.normal = normal;
            b.normal = normal;
            c.normal = normal;
        }

        cube2.reset(gint->newPrimitive(vertices, indices, &*texture2));
    }

    perspective.reset(gint->newView());
    light.reset(gint->newLight());

    debug("game init complete");
}

void renderTick() {

    {
        float fov = 70.0f;

        auto extent = gint->getViewport();
        auto proj = glm::perspective(glm::radians(fov),
                                     extent.x / (float)extent.y, 0.1f, 10.0f);
        proj[1][1] *= -1;

        auto view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f),
                                glm::vec3(0.0f, 0.0f, 0.0f),
                                glm::vec3(0.0f, 0.0f, 1.0f));

        perspective->configure(proj, view);
    }

    perspective->use();

    float contrast = glm::sin(gint->tmp_getTime() / 3) * 0.18f + 0.18f;
    glm::vec3 color0(0.60f, 0.60f, 0.70f);
    glm::vec3 color1(1.10f, 1.05f, 0.70f);

    float m = glm::sin(gint->tmp_getTime() / 3) * 0.5 + 0.5;
    glm::vec3 color = m * color1 + (1 - m) * color0;

    light->configure(color, glm::vec3(1.0f, -2.0f, 1.0f), contrast, 0.1f);
    light->use();

    auto model = glm::eulerAngleYXZ(0.0f, 0.0f, gint->tmp_getTime() * 0.1f);

    gint->setModelTransform(model);
    cube1->draw();
    cube2->draw();
}

void shutdown() {
    debug("game shutdown begin");

    cube1.reset();
    cube2.reset();
    texture1.reset();
    texture2.reset();

    light.reset();
    perspective.reset();

    debug("game shutdown complete");
}

} // namespace main
} // namespace progressia

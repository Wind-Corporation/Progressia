#include "game.h"

#include <array>
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

namespace progressia::main {

class GameImpl : public Game {

    DISABLE_COPYING(GameImpl)
    DISABLE_MOVING(GameImpl)

  public:
    std::unique_ptr<Primitive> cube1;
    std::unique_ptr<Primitive> cube2;
    std::unique_ptr<Texture> texture1;
    std::unique_ptr<Texture> texture2;
    std::unique_ptr<View> perspective;
    std::unique_ptr<Light> light;

    GraphicsInterface *gint;

    static void addRect(glm::vec3 origin, glm::vec3 width, glm::vec3 height,
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

    static void addBox(glm::vec3 origin, glm::vec3 length, glm::vec3 height,
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

    GameImpl(GraphicsInterface &gintp) {

        debug("game init begin");
        gint = &gintp;

        texture1 =
            gint->newTexture(progressia::main::loadImage("assets/texture.png"));
        texture2 = gint->newTexture(
            progressia::main::loadImage("assets/texture2.png"));

        // Cube 1
        {
            std::vector<Vertex> vertices;
            std::vector<Vertex::Index> indices;
            auto white = glm::vec4(1, 1, 1, 1);

            addBox({-0.5, -0.5, -0.5}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
                   {white, white, white, white, white, white}, vertices,
                   indices);

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

            cube1 = gint->newPrimitive(vertices, indices, &*texture1);
        }

        // Cube 2
        {
            std::vector<Vertex> vertices;
            std::vector<Vertex::Index> indices;
            auto white = glm::vec4(1, 1, 1, 1);

            addBox({-0.5, -2.5, -0.5}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
                   {white, white, white, white, white, white}, vertices,
                   indices);

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

            cube2 = gint->newPrimitive(vertices, indices, &*texture2);
        }

        perspective = gint->newView();
        light = gint->newLight();

        debug("game init complete");
    }

    void renderTick() override {

        {
            float fov = 70.0F;

            auto extent = gint->getViewport();
            auto proj = glm::perspective(
                glm::radians(fov), extent.x / (float)extent.y, 0.1F, 10.0F);
            proj[1][1] *= -1;

            auto view = glm::lookAt(glm::vec3(2.0F, 2.0F, 2.0F),
                                    glm::vec3(0.0F, 0.0F, 0.0F),
                                    glm::vec3(0.0F, 0.0F, 1.0F));

            perspective->configure(proj, view);
        }

        perspective->use();

        float contrast = glm::sin(gint->tmp_getTime() / 3) * 0.18F + 0.18F;
        glm::vec3 color0(0.60F, 0.60F, 0.70F);
        glm::vec3 color1(1.10F, 1.05F, 0.70F);

        auto m =
            static_cast<float>(glm::sin(gint->tmp_getTime() / 3) * 0.5 + 0.5);
        glm::vec3 color = m * color1 + (1 - m) * color0;

        light->configure(color, glm::vec3(1.0F, -2.0F, 1.0F), contrast, 0.1F);
        light->use();

        auto model = glm::eulerAngleYXZ(0.0F, 0.0F, gint->tmp_getTime() * 0.1F);

        gint->setModelTransform(model);
        cube1->draw();
        cube2->draw();
    }

    ~GameImpl() override {
        debug("game shutdown begin");

        cube1.reset();
        cube2.reset();
        texture1.reset();
        texture2.reset();

        light.reset();
        perspective.reset();

        debug("game shutdown complete");
    }
};

std::unique_ptr<Game> makeGame(GraphicsInterface &gint) {
    return std::make_unique<GameImpl>(gint);
}

} // namespace progressia::main

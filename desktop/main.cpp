#include <iostream>

#include "../main/game.h"
#include "graphics/glfw_mgmt.h"
#include "graphics/vulkan_mgmt.h"

int main() {

    using namespace progressia;

    desktop::initializeGlfw();
    desktop::initializeVulkan();
    desktop::showWindow();

    main::initialize(desktop::getVulkan()->getGint());

    while (desktop::shouldRun()) {
        bool abortFrame = !desktop::startRender();
        if (abortFrame) {
            continue;
        }

        main::renderTick();

        desktop::endRender();
        desktop::doGlfwRoutine();
    }

    desktop::getVulkan()->waitIdle();
    main::shutdown();
    desktop::shutdownVulkan();
    desktop::shutdownGlfw();

    return 0;
}

#include <iostream>

#include "../main/game.h"
#include "../main/logging.h"
#include "../main/meta.h"
#include "graphics/glfw_mgmt.h"
#include "graphics/vulkan_mgmt.h"

using namespace progressia::main::logging;

int main(int argc, char *argv[]) {

    using namespace progressia;

    for (int i = 1; i < argc; i++) {
        char *arg = argv[i];
        if (strcmp(arg, "--version") == 0 || strcmp(arg, "-v") == 0) {
            std::cout << main::meta::NAME << " " << main::meta::VERSION << "+"
                      << main::meta::BUILD_ID << " (version number "
                      << main::meta::VERSION_NUMBER << ")" << std::endl;
            return 0;
        }
    }

    info() << "Starting " << main::meta::NAME << " " << main::meta::VERSION
           << "+" << main::meta::BUILD_ID << " (version number "
           << main::meta::VERSION_NUMBER << ")";
    debug("Debug is enabled");

    desktop::initializeGlfw();
    desktop::initializeVulkan();
    desktop::showWindow();

    main::initialize(desktop::getVulkan()->getGint());

    info("Loading complete");
    while (desktop::shouldRun()) {
        bool abortFrame = !desktop::startRender();
        if (abortFrame) {
            continue;
        }

        main::renderTick();

        desktop::endRender();
        desktop::doGlfwRoutine();
    }
    info("Shutting down");

    desktop::getVulkan()->waitIdle();
    main::shutdown();
    desktop::shutdownVulkan();
    desktop::shutdownGlfw();

    return 0;
}

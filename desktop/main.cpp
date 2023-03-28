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

    auto glfwManager = desktop::makeGlfwManager();
    desktop::VulkanManager vulkanManager;
    glfwManager->setOnScreenResize([&]() { vulkanManager.resizeSurface(); });
    glfwManager->showWindow();

    auto game = main::makeGame(vulkanManager.getVulkan()->getGint());

    info("Loading complete");
    while (glfwManager->shouldRun()) {
        bool abortFrame = !vulkanManager.startRender();
        if (abortFrame) {
            continue;
        }

        game->renderTick();

        vulkanManager.endRender();
        glfwManager->doGlfwRoutine();
    }
    info("Shutting down");

    vulkanManager.getVulkan()->waitIdle();

    return 0;
}

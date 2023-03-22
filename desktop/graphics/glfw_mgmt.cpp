#include "glfw_mgmt_details.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

#include "../../main/logging.h"
#include "../../main/meta.h"
#include "vulkan_mgmt.h"
using namespace progressia::main::logging;

namespace progressia::desktop {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): global variable required for GLFW callbacks
static GLFWwindow *window = nullptr;

static void onGlfwError(int errorCode, const char *description);
static void onWindowGeometryChange(GLFWwindow *window, int width, int height);

void initializeGlfw() {
    debug("Beginning GLFW init");

    glfwSetErrorCallback(onGlfwError);

    if (!glfwInit()) {
        fatal("glfwInit() failed");
        // REPORT_ERROR
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    std::string title;

    {
        std::stringstream accumulator;
        accumulator << progressia::main::meta::NAME << " "
                    << progressia::main::meta::VERSION << " build "
                    << progressia::main::meta::BUILD_ID;
        title = accumulator.str();
    }

    constexpr auto windowDimensions = 800;
    window = glfwCreateWindow(windowDimensions, windowDimensions, title.c_str(),
                              nullptr, nullptr);

    glfwSetWindowSizeCallback(window, onWindowGeometryChange);

    debug("GLFW init complete");
}

void showWindow() {
    glfwShowWindow(window);
    debug("Window now visible");
}

bool shouldRun() { return !glfwWindowShouldClose(window); }

void doGlfwRoutine() { glfwPollEvents(); }

void shutdownGlfw() { glfwTerminate(); }

void onGlfwError(int errorCode, const char *description) {
    fatal() << "[GLFW] " << description << " (" << errorCode << ")";
    // REPORT_ERROR
    exit(1);
}

void onWindowGeometryChange(GLFWwindow *window, [[maybe_unused]] int width,
                            [[maybe_unused]] int height) {
    if (window != progressia::desktop::window) {
        return;
    }

    resizeVulkanSurface();
}

GLFWwindow *getGLFWWindowHandle() { return window; }

} // namespace progressia::desktop

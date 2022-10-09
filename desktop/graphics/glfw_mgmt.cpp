#include "glfw_mgmt_details.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>

#include "vulkan_mgmt.h"

namespace progressia {
namespace desktop {

static GLFWwindow *window = nullptr;

static void onGlfwError(int errorCode, const char *description);
static void onWindowGeometryChange(GLFWwindow *window, int width, int height);

void initializeGlfw() {
    std::cout << "Beginning GLFW init" << std::endl;

    glfwSetErrorCallback(onGlfwError);

    if (!glfwInit()) {
        std::cout << "glfwInit() failed" << std::endl;
        // REPORT_ERROR
        exit(1);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    window = glfwCreateWindow(800, 800, "Progressia", nullptr, nullptr);

    glfwSetWindowSizeCallback(window, onWindowGeometryChange);

    std::cout << "GLFW init complete" << std::endl;
}

void showWindow() {
    glfwShowWindow(window);
    std::cout << "Window now visible" << std::endl;
}

bool shouldRun() { return !glfwWindowShouldClose(window); }

void doGlfwRoutine() { glfwPollEvents(); }

void shutdownGlfw() { glfwTerminate(); }

void onGlfwError(int errorCode, const char *description) {
    std::cout << "[GLFW] " << description << " (" << errorCode << ")"
              << std::endl;
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

} // namespace desktop
} // namespace progressia

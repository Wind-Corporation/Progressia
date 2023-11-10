#include "glfw_mgmt_details.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <sstream>

#include "../../main/logging.h"
#include "../../main/meta.h"
#include "../../main/util.h"

using namespace progressia::main::logging;

namespace progressia::desktop {

static void onGlfwError(int errorCode, const char *description);
static void onWindowGeometryChange(GLFWwindow *window, int width, int height);

class GlfwManagerImpl : public GlfwManager {
  private:
    GLFWwindow *window = nullptr;
    std::function<void()> onScreenResize = nullptr;

  public:
    DISABLE_COPYING(GlfwManagerImpl)
    DISABLE_MOVING(GlfwManagerImpl)

    GlfwManagerImpl() {
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
        window = glfwCreateWindow(windowDimensions, windowDimensions,
                                  title.c_str(), nullptr, nullptr);

        glfwSetWindowSizeCallback(window, onWindowGeometryChange);

        debug("GLFW init complete");
    }

    ~GlfwManagerImpl() override { glfwTerminate(); }

    void setOnScreenResize(std::function<void()> hook) override {
        onScreenResize = hook;
    }

    void showWindow() override {
        glfwShowWindow(window);
        debug("Window now visible");
    }

    bool shouldRun() override { return !glfwWindowShouldClose(window); }

    void doGlfwRoutine() override { glfwPollEvents(); }

    friend GLFWwindow *getGLFWWindowHandle();
    friend void onWindowGeometryChange(GLFWwindow *, int, int);
};

namespace {
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables): global variables required by GLFW C callbacks
std::weak_ptr<GlfwManagerImpl> theGlfwManager;
} // namespace

std::shared_ptr<GlfwManager> makeGlfwManager() {
    if (!theGlfwManager.expired()) {
        fatal("GlfwManager already exists");
        // REPORT_ERROR
        exit(1);
    }

    std::shared_ptr<GlfwManagerImpl> aGlfwManager =
        std::make_shared<GlfwManagerImpl>();

    theGlfwManager = aGlfwManager;
    return aGlfwManager;
}

void onGlfwError(int errorCode, const char *description) {
    fatal() << "[GLFW] " << description << " (" << errorCode << ")";
    // REPORT_ERROR
    exit(1);
}

void onWindowGeometryChange(GLFWwindow *window, [[maybe_unused]] int width,
                            [[maybe_unused]] int height) {
    if (auto manager = theGlfwManager.lock()) {
        if (manager->window != window) {
            return;
        }

        if (manager->onScreenResize != nullptr) {
            manager->onScreenResize();
        }
    } else {
        return;
    }
}

GLFWwindow *getGLFWWindowHandle() {
    if (auto manager = theGlfwManager.lock()) {
        return manager->window;
    }

    return nullptr;
}

} // namespace progressia::desktop

#pragma once

#include <functional>
#include <memory>

namespace progressia::desktop {

class GlfwManager {

  public:
    virtual ~GlfwManager(){};

    virtual void setOnScreenResize(std::function<void()>) = 0;

    virtual void showWindow() = 0;
    virtual bool shouldRun() = 0;
    virtual void doGlfwRoutine() = 0;
};

std::shared_ptr<GlfwManager> makeGlfwManager();

} // namespace progressia::desktop

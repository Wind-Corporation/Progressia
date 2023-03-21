#pragma once

#include "glfw_mgmt.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace progressia::desktop {

GLFWwindow *getGLFWWindowHandle();

} // namespace progressia::desktop

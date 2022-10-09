#pragma once

namespace progressia {
namespace desktop {

void initializeGlfw();
void showWindow();
void shutdownGlfw();
bool shouldRun();
void doGlfwRoutine();

} // namespace desktop
} // namespace progressia

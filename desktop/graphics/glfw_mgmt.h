#pragma once

namespace progressia::desktop {

void initializeGlfw();
void showWindow();
void shutdownGlfw();
bool shouldRun();
void doGlfwRoutine();

} // namespace progressia::desktop

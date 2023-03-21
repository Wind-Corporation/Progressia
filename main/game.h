#pragma once

#include "rendering.h"

namespace progressia::main {

void initialize(GraphicsInterface &);
void renderTick();
void shutdown();

} // namespace progressia::main

#pragma once

#include "rendering.h"
#include "util.h"

namespace progressia::main {

class Game : private NonCopyable {
  public:
    virtual ~Game() = default;
    virtual void renderTick() = 0;
};

std::unique_ptr<Game> makeGame(GraphicsInterface &);

} // namespace progressia::main

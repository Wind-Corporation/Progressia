#pragma once

#include <cstdlib>

namespace progressia {
namespace main {
namespace meta {

constexpr const char *NAME = "Progressia";

#ifndef _MAJOR
#warning Version number (_MAJOR _MINOR _PATCH _BUILD) not set, using 0.0.0+1
#define _MAJOR 0
#define _MINOR 0
#define _PATCH 0
#define _BUILD 1
#endif

using VersionUnit = uint8_t;
using VersionInt = uint32_t;

constexpr struct {

    VersionUnit major, minor, patch, build;

    VersionInt number;

    bool isRelease;

} VERSION{_MAJOR,
          _MINOR,
          _PATCH,
          _BUILD,

          (static_cast<VersionInt>(_MAJOR) << 24) |
              (static_cast<VersionInt>(_MINOR) << 16) |
              (static_cast<VersionInt>(_PATCH) << 8) |
              (static_cast<VersionInt>(_BUILD) << 0),

          _BUILD == 0};

} // namespace meta
} // namespace main
} // namespace progressia

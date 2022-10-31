#pragma once

#include "../config.h"
#include <cstdlib>

namespace progressia {
namespace main {
namespace meta {

namespace detail {
constexpr static uint32_t getVersionNumber(const char *versionStr) {
    uint32_t parts[] = {0, 0, 0};
    std::size_t partCount = sizeof(parts) / sizeof(parts[0]);
    std::size_t currentPart = 0;

    for (const char *cp = versionStr; *cp != '\0'; cp++) {
        char c = *cp;
        if (c == '.') {
            currentPart++;
        } else if (currentPart < partCount && c >= '0' && c <= '9') {
            parts[currentPart] =
                parts[currentPart] * 10 + static_cast<uint32_t>(c - '0');
        }
    }

    return (parts[0] << 16) | (parts[1] << 8) | (parts[2] << 0);
}
} // namespace detail

constexpr const char *NAME = "Progressia";
constexpr const char *VERSION = _VERSION;
constexpr const char *BUILD_ID = _BUILD_ID;

constexpr uint32_t VERSION_NUMBER = detail::getVersionNumber(_VERSION);
constexpr uint32_t VERSION_MAJOR = (VERSION_NUMBER & 0xFF0000) >> 16;
constexpr uint32_t VERSION_MINOR = (VERSION_NUMBER & 0x00FF00) >> 8;
constexpr uint32_t VERSION_PATCH = (VERSION_NUMBER & 0x0000FF) >> 0;

} // namespace meta
} // namespace main
} // namespace progressia

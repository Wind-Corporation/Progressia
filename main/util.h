#pragma once

// clang-format off
#define FOR_PACK(PACK_TYPE, PACK_NAME, VAR, CODE) \
{                                                 \
    [[maybe_unused]] int dummy[] {                \
        (                                         \
            [&](PACK_TYPE VAR) {                  \
                CODE;                             \
                return 0;                         \
            }                                     \
        )(PACK_NAME)...                           \
    };                                            \
}
// clang-format on

// clang-format off
#define FOR_PACK_S(PACK_TYPE, VAR_TYPE, CODE) \
{                                             \
    [[maybe_unused]] int dummy[] {            \
        (                                     \
            [&]() {                           \
                using VAR_TYPE = PACK_TYPE;   \
                CODE;                         \
                return 0;                     \
            }                                 \
        )()...                                \
    };                                        \
}
// clang-format on

// clang-format off
#define DISABLE_MOVING(CLASS)            \
    CLASS &operator=(CLASS &&) = delete; \
    CLASS(CLASS &&) = delete;            \
// clang-format on

// clang-format off
#define DISABLE_COPYING(CLASS)                \
    CLASS &operator=(const CLASS &) = delete; \
    CLASS(const CLASS &) = delete;            \
// clang-format on

namespace progressia::main {

struct NonCopyable {
    NonCopyable &operator=(const NonCopyable &) = delete;
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable() = default;
};

} // namespace progressia::main

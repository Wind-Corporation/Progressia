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

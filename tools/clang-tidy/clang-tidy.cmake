if (DEV_MODE)
    find_program(clang_tidy_EXECUTABLE NAMES clang-tidy-13 clang-tidy)

    #list(APPEND clang_tidy_command "${clang_tidy_EXECUTABLE}")
    message(NOTICE ${clang_tidy_EXECUTABLE})

    add_custom_command(TARGET progressia PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
        "Clang-tidy is enabled")


endif()

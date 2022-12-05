if (DEV_MODE)
    find_program(clang_tidy_EXECUTABLE NAMES clang-tidy-13 clang-tidy)

    list(APPEND clang_tidy_command "${clang_tidy_EXECUTABLE}"
        "--config-file=${CMAKE_CURRENT_LIST_DIR}/clang-tidy.yml"
        "--format-style=${clang_format_style}"
        "--warnings-as-errors=*"
        "--use-color")

    set_target_properties(progressia
        PROPERTIES CXX_CLANG_TIDY "${clang_tidy_command}")

    add_custom_command(TARGET progressia PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo
        "Clang-tidy is enabled")

endif()

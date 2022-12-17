if (DEV_MODE)
    find_program(clang_tidy_EXECUTABLE NAMES clang-tidy-13 clang-tidy)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    
    # Setup clang-tidy
    set(clang_tidy_config_file "${CMAKE_CURRENT_LIST_DIR}/clang-tidy.yml")
    list(APPEND clang_tidy_command "${clang_tidy_EXECUTABLE}"
        "--config-file=${clang_tidy_config_file}"
        "--warnings-as-errors=*"
        "--use-color")

    set_target_properties(progressia
        PROPERTIES CXX_CLANG_TIDY "${clang_tidy_command}")

    # Display the marker for pre-commit.py at build time
    add_custom_target(clang_tidy_marker ALL
        COMMAND ${CMAKE_COMMAND} -E echo
        "Clang-tidy is enabled (this is a marker for pre-commit.py)")
    
    # Notify pre-commit.py about CMAKE_BINARY_DIR
    execute_process(COMMAND ${Python3_EXECUTABLE} ${tools}/pre-commit.py
            set-build-root -- "${CMAKE_BINARY_DIR}"
            RESULT_VARIABLE set_build_root_RESULT)
    
    if(${set_build_root_RESULT})
        message(FATAL_ERROR "pre-commit.py set-build-root failed")
    endif()
    
    # Setup pre-commit git hook
    if (IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git/hooks")
        if (NOT EXISTS "${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit")
            file(WRITE "${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit"
                "#!/bin/bash\n"
                "# Progressia autogenerated pre-commit hook\n"
                "# You may modify this hook freely "
                "(just make sure the checks run)\n"
                "/bin/env python3 ${CMAKE_SOURCE_DIR}/tools/pre-commit.py run")
        endif()
    endif()
    
endif()
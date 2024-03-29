if (DEV_MODE)
    find_program(clang_tidy_EXECUTABLE NAMES clang-tidy-13 clang-tidy REQUIRED)
    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    # Setup clang-tidy
    list(APPEND clang_tidy_command "${clang_tidy_EXECUTABLE}"
        "--warnings-as-errors=*"
        "--use-color")

    set_target_properties(progressia
        PROPERTIES CXX_CLANG_TIDY "${clang_tidy_command}")

    # Display the marker for pre-commit.py at build time
    add_custom_target(clang_tidy_marker ALL
        COMMAND ${CMAKE_COMMAND} -E echo
        "Clang-tidy is enabled. This is a marker for pre-commit.py")

    # Notify pre-commit.py about CMake settings
    execute_process(COMMAND ${Python3_EXECUTABLE} ${tools}/pre-commit.py
        set-build-info -- "${CMAKE_COMMAND}" "${CMAKE_BINARY_DIR}"
        RESULT_VARIABLE set_build_info_RESULT)

    if(${set_build_info_RESULT})
        message(FATAL_ERROR "pre-commit.py set-build-info failed")
    endif()

    # Setup pre-commit git hook
    if (IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git/hooks")
        set(pre_commit_hook "${CMAKE_SOURCE_DIR}/.git/hooks/pre-commit")
        if (NOT EXISTS "${pre_commit_hook}")
            file(WRITE "${pre_commit_hook}"
                "#!/bin/sh\n"
                "# Progressia autogenerated pre-commit hook\n"
                "# You may modify this hook freely "
                "(just make sure the checks run)\n"
                "/bin/env python3 ${CMAKE_SOURCE_DIR}/tools/pre-commit.py run")

            if (${CMAKE_VERSION} VERSION_LESS "3.19.0")
                if (${CMAKE_HOST_UNIX})
                    execute_process(COMMAND chmod "755" "${pre_commit_hook}"
                        RESULT_VARIABLE chmod_RESULT)
                    if (${chmod_RESULT})
                        message(FATAL_ERROR "Could not make git pre-commit hook executable")
                    endif()
                endif()
            else()
                file(CHMOD "${pre_commit_hook}"
                    PERMISSIONS
                    OWNER_READ OWNER_WRITE OWNER_EXECUTE
                    GROUP_READ GROUP_EXECUTE
                    WORLD_READ WORLD_EXECUTE)
            endif()
        endif()
        unset(pre_commit_hook)
    endif()

endif()

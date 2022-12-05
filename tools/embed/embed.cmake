# embed.cmake
# Generates embedded_resources.h and embedded_resources.cpp

find_package(Vulkan COMPONENTS glslc REQUIRED)
find_program(glslc_EXECUTABLE NAMES glslc HINTS Vulkan::glslc)

find_package(Python3 COMPONENTS Interpreter REQUIRED)

macro (get_target_property_or var target prop default)
    get_property(__is_set TARGET ${target} PROPERTY ${prop} SET)
    if (__is_set)
        get_property(${var} TARGET ${target} PROPERTY ${prop})
    else()
        set(${var} "${default}")
    endif()
    unset(__is_set)
endmacro()

function (target_embeds)
    set(expecting_name FALSE)
    set(target "")
    set(current_asset "")

    foreach (word ${ARGV})

        # First argument is target name
        if (target STREQUAL "")
            set(target "${word}")
            get_target_property_or(script_args "${target}" EMBED_ARGS "")
            get_target_property_or(embeds "${target}" EMBEDS "")
            continue()
        endif()

        if (current_asset STREQUAL "")
            # Beginning of asset declaration (1/2)
            set(current_asset "${word}")

        elseif (expecting_name)
            # End of "asset AS asset_name"
            list(APPEND script_args "${current_asset};as;${word}")
            list(APPEND embeds ${current_asset})
            set(current_asset "")
            set(expecting_name FALSE)

        elseif ("${word}" STREQUAL "AS")
            # Keyword AS in "asset AS asset_name"
            set(expecting_name TRUE)

        else()
            # End of asset without AS, beginning of asset declaration (2/2)
            list(APPEND script_args "${current_asset};as;${current_asset}")
            list(APPEND embeds ${current_asset})
            set(current_asset "${word}")
        endif()

    endforeach()

    if (expecting_name)
        message(FATAL_ERROR "No name given for asset \"${current_asset}\"")
    endif()

    if (NOT current_asset STREQUAL "")
        list(APPEND script_args "${current_asset};as;${current_asset}")
    endif()

    set_target_properties("${target}" PROPERTIES EMBED_ARGS "${script_args}")
    set_target_properties("${target}" PROPERTIES EMBEDS "${embeds}")
endfunction()

file(MAKE_DIRECTORY "${generated}/embedded_resources")

function(compile_embeds target)
    get_target_property(script_args "${target}" EMBED_ARGS)
    get_target_property(embeds "${target}" EMBEDS)

    add_custom_command(
        OUTPUT  ${generated}/embedded_resources/embedded_resources.cpp
                ${generated}/embedded_resources/embedded_resources.h

        COMMAND ${Python3_EXECUTABLE} ${tools}/embed/embed.py
                --cpp    ${generated}/embedded_resources/embedded_resources.cpp
                --header ${generated}/embedded_resources/embedded_resources.h
                --
                ${script_args}

        DEPENDS ${embeds}
                ${tools}/embed/embed.py

        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        COMMENT "Embedding assets"
    )
endfunction()

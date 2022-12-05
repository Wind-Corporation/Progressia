# glslc.cmake
# Compiles GLSL shaders to SPV files

find_package(Vulkan COMPONENTS glslc REQUIRED)
find_program(glslc_EXECUTABLE NAMES glslc HINTS Vulkan::glslc)

macro (get_target_property_or var target prop default)
    get_property(__is_set TARGET ${target} PROPERTY ${prop} SET)
    if (__is_set)
        get_property(${var} TARGET ${target} PROPERTY ${prop})
    else()
        set(${var} "${default}")
    endif()
    unset(__is_set)
endmacro()

function (target_glsl_shaders)
    set(target "")

    foreach (word ${ARGV})
        # First argument is target name
        if (target STREQUAL "")
            set(target ${word})
            get_target_property_or(glsl_shaders ${target} GLSL_SHADERS "")
        else()
            list(APPEND glsl_shaders ${word})
        endif()
    endforeach()

    set_target_properties(${target} PROPERTIES GLSL_SHADERS "${glsl_shaders}")
endfunction()

file(MAKE_DIRECTORY "${generated}/compiled_glsl_shaders")

function(compile_glsl target)
    get_target_property(glsl_shaders ${target} GLSL_SHADERS)

    foreach (source_path ${glsl_shaders})
        get_filename_component(source_basename ${source_path} NAME)
        set(spv_path
            "${generated}/compiled_glsl_shaders/${source_basename}.spv")

        add_custom_command(
            OUTPUT ${spv_path}
            DEPENDS ${source_path}
            COMMAND ${glslc_EXECUTABLE}
                    -o ${spv_path}
                    ${CMAKE_CURRENT_SOURCE_DIR}/${source_path}
            COMMENT "Compiling shader ${source_path}"
        )
        target_embeds(${target} ${spv_path} AS "${source_basename}.spv")
    endforeach()
endfunction()

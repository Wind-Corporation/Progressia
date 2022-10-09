# Global variables. Yikes. FIXME
set(tools ${PROJECT_SOURCE_DIR}/tools)
set(generated ${PROJECT_BINARY_DIR}/generated)
set(assets_to_embed "")
set(assets_to_embed_args "")

file(MAKE_DIRECTORY ${generated})

find_package(Vulkan COMPONENTS glslc REQUIRED)
find_program(glslc_executable NAMES glslc HINTS Vulkan::glslc)
set(shaders ${generated}/shaders)
file(MAKE_DIRECTORY ${shaders})

# Shedules compilation of shaders
# Adapted from https://stackoverflow.com/a/60472877/4463352
macro(compile_shader)
    foreach(source ${ARGV})
        get_filename_component(source_basename ${source} NAME)
        set(tmp "${shaders}/${source_basename}.spv")
        add_custom_command(
            OUTPUT ${tmp}
            DEPENDS ${source}
            COMMAND ${glslc_executable}
                    -o ${tmp}
                    ${CMAKE_CURRENT_SOURCE_DIR}/${source}
            COMMENT "Compiling shader ${source}"
        )
        list(APPEND assets_to_embed_args "${tmp};as;${source_basename}.spv")
        list(APPEND assets_to_embed "${tmp}")
        unset(tmp)
        unset(source_basename)
    endforeach()
endmacro()

compile_shader(
    desktop/graphics/shaders/shader.frag
    desktop/graphics/shaders/shader.vert
)

# Generate embed files
add_custom_command(
    OUTPUT ${generated}/embedded_resources.cpp
           ${generated}/embedded_resources.h

    COMMAND ${tools}/embed/embed.py
            --cpp    ${generated}/embedded_resources.cpp
            --header ${generated}/embedded_resources.h
            --
            ${assets_to_embed_args}

    DEPENDS "${assets_to_embed}"
            ${tools}/embed/embed.py

    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Embedding assets"
)

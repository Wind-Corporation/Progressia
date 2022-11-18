cmake_minimum_required(VERSION 3.12)

project(progressia)
set(VERSION "0.0.1")

set(tools ${PROJECT_SOURCE_DIR}/tools)
set(generated ${PROJECT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY "${generated}")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/")
include(embed/embed)
include(glslc)

# Options
option(VULKAN_ERROR_CHECKING "Enable Vulkan validation layers to detect Vulkan API usage errors at runtime")

# Source files
add_executable(progressia
    desktop/main.cpp
    desktop/graphics/glfw_mgmt.cpp
    desktop/graphics/vulkan_common.cpp
    desktop/graphics/vulkan_frame.cpp
    desktop/graphics/vulkan_image.cpp
    desktop/graphics/vulkan_mgmt.cpp
    desktop/graphics/vulkan_pick_device.cpp
    desktop/graphics/vulkan_pipeline.cpp
    desktop/graphics/vulkan_render_pass.cpp
    desktop/graphics/vulkan_descriptor_set.cpp
    desktop/graphics/vulkan_texture_descriptors.cpp
    desktop/graphics/vulkan_adapter.cpp
    desktop/graphics/vulkan_swap_chain.cpp
    desktop/graphics/vulkan_physical_device.cpp

    main/game.cpp
    main/logging.cpp

    main/rendering/image.cpp

    main/stb_image.c
    ${generated}/embedded_resources/embedded_resources.cpp
)

# Embedded resources
target_glsl_shaders(progressia
    desktop/graphics/shaders/shader.frag
    desktop/graphics/shaders/shader.vert)
compile_glsl(progressia)
compile_embeds(progressia)
target_include_directories(progressia PRIVATE ${generated}/embedded_resources)

# Compilation settings

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(gcc_like_compiler TRUE)
else()
    set(gcc_like_compiler FALSE)
endif()

set_property(TARGET progressia PROPERTY CXX_STANDARD 17)
set_property(TARGET progressia PROPERTY CXX_STANDARD_REQUIRED ON)

# Do Windows-specific tweaks for release builds
if (WIN32 AND DEFINED ENV{PROGRESSIA_BUILD_ID})
    set_target_properties(progressia PROPERTIES WIN32_EXECUTABLE true)

    if (gcc_like_compiler)
        target_link_options(progressia PRIVATE -static-libstdc++ -static-libgcc)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_link_options(progressia PRIVATE /entry:mainCRTStartup)
        message(WARNING "Release builds with MSVC are not supported")
    endif()
endif()

# Extra options for gcc -- we're using this for extra static analisys for now
if (gcc_like_compiler)
    target_compile_options(progressia PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()

# Pass configuration options
if (DEFINED ENV{PROGRESSIA_BUILD_ID})
    set(BUILD_ID ENV{PROGRESSIA_BUILD_ID})
else()
    set(BUILD_ID "dev")
endif()
file(MAKE_DIRECTORY "${generated}/config")
configure_file(${PROJECT_SOURCE_DIR}/main/config.h.in
               ${generated}/config/config.h)
target_include_directories(progressia PRIVATE ${generated}/config)

# Libraries

# Use Vulkan
find_package(Vulkan 1.0 REQUIRED)
target_link_libraries(progressia Vulkan::Vulkan)

# Use GLFW3
find_package(glfw3 3.3.2 REQUIRED)
target_link_libraries(progressia glfw)

# Use GLM
find_package(glm REQUIRED) # glmConfig-version.cmake is broken
target_link_libraries(progressia glm::glm)

# Use STB
target_include_directories(progressia PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lib/stb/include)

# Use Boost
find_package(Boost 1.74 REQUIRED)
target_link_libraries(progressia Boost::headers)

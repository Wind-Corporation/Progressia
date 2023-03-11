cmake_minimum_required(VERSION 3.12)

project(progressia)
set(VERSION "0.0.1")
add_executable(progressia)

# Options

option(DEV_MODE "Enable additional functionality required for development.")

string(CONCAT BUILD_ID_expl
    "Build ID or \"dev\".\n"
    "Set to a unique identifying string if you intend to publish your builds.")
set(BUILD_ID "dev" CACHE STRING "${BUILD_ID_expl}")

string(CONCAT VULKAN_ERROR_CHECKING_expl
    "Enable Vulkan validation layers to detect Vulkan API usage errors "
    "at runtime.\n"
    "Requires Vulkan SDK. This will lead to decreased performance.")
option(VULKAN_ERROR_CHECKING "${VULKAN_ERROR_CHECKING_expl}")

# Tools

set(tools ${PROJECT_SOURCE_DIR}/tools)
set(generated ${PROJECT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY "${generated}")

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/")
include(embed/embed)
include(glslc)
include(clang-tidy/clang-tidy)

# Source files
target_sources(progressia PRIVATE
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

target_embeds(progressia
    assets/texture.png
    assets/texture2.png)

compile_glsl(progressia)
compile_embeds(progressia)
target_include_directories(progressia PRIVATE ${generated}/embedded_resources)

# Compilation settings

set_property(TARGET progressia PROPERTY CXX_STANDARD 17)
set_property(TARGET progressia PROPERTY CXX_STANDARD_REQUIRED ON)

# Determine command line style
if (DEFINED compiler_cl_dialect)
    # Do nothing
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(compiler_cl_dialect "GCC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(compiler_cl_dialect "MSVC")
elseif (CMAKE_CXX_SIMULATE_ID STREQUAL "GCC")
    set(compiler_cl_dialect "GCC")
elseif (CMAKE_CXX_SIMULATE_ID STREQUAL "MSVC")
    set(compiler_cl_dialect "MSVC")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # On Linux, clang does not have SIMULATE_ID
    set(compiler_cl_dialect "GCC")
elseif(WIN32)
    message(WARNING "Could not determine compiler command line dialect, guessing MSVC")
    set(compiler_cl_dialect "MSVC")
else()
    message(WARNING "Could not determine compiler command line dialect, guessing GCC")
    set(compiler_cl_dialect "GCC")
endif()

# Do Windows-specific tweaks for release builds
if (WIN32 AND NOT BUILD_ID STREQUAL "dev")
    set_target_properties(progressia PROPERTIES WIN32_EXECUTABLE true)

    if (compiler_cl_dialect STREQUAL "GCC")
        target_link_options(progressia PRIVATE -static-libstdc++ -static-libgcc)
    elseif (compiler_cl_dialect STREQUAL "MSVC")
        target_link_options(progressia PRIVATE /entry:mainCRTStartup)
        message(WARNING "Release builds with MSVC are not supported")
    endif()
endif()

# Pass configuration options
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

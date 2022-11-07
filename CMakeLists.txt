cmake_minimum_required(VERSION 3.10)

project(progressia)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/tools/cmake")
include(embed)

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
    ${generated}/embedded_resources.cpp
)

target_include_directories(progressia PRIVATE ${generated})

# Do Windows-specific tweaks
if (WIN32)
    set_target_properties(progressia PROPERTIES WIN32_EXECUTABLE true)
    target_link_options(progressia PRIVATE -static-libstdc++ -static-libgcc)
endif()

# Compilation settings
set_property(TARGET progressia PROPERTY CXX_STANDARD 17)
target_compile_options(progressia PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Version information
if (NOT DEFINED BUILD_ID)
    set(BUILD_ID "dev")
endif()

set(VERSION "0.0.1")

# Debug options
option(VULKAN_ERROR_CHECKING "Enable Vulkan validation layers to detect Vulkan API usage errors at runtime")

# Pass configuration options
configure_file(${PROJECT_SOURCE_DIR}/main/config.h.in
               ${PROJECT_BINARY_DIR}/config.h)

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

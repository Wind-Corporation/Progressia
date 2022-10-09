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

	main/game.cpp

	main/rendering/image.cpp

	main/stb_image.c
	${generated}/embedded_resources.cpp
)

target_include_directories(progressia PRIVATE ${generated})

# Compilation settings
set_property(TARGET progressia PROPERTY CXX_STANDARD 17)
target_compile_options(progressia PRIVATE -Wall -Wextra -Wpedantic -Werror)

# Pass version information
target_compile_definitions(progressia PRIVATE
                           _MAJOR=0 _MINOR=0 _PATCH=0 _BUILD=1)

# Debug options
option(VULKAN_ERROR_CHECKING "Enable Vulkan validation layers to detect Vulkan API usage errors at runtime")
if (VULKAN_ERROR_CHECKING)
	target_compile_definitions(progressia PRIVATE VULKAN_ERROR_CHECKING)
endif()

# Libraries

find_package(PkgConfig REQUIRED)

# Use Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(progressia ${Vulkan_LIBRARIES})
target_include_directories(progressia PUBLIC ${Vulkan_INCLUDE_DIRS})

# Use GLFW3
find_package(glfw3 REQUIRED)
target_link_libraries(progressia glfw)

# Use GLM
pkg_check_modules(GLM REQUIRED glm)
target_link_libraries(progressia ${GLM_LIBRARIES})
target_include_directories(progressia PUBLIC ${GLM_INCLUDE_DIRS})
target_compile_options(progressia PUBLIC ${GLM_CFLAGS_OTHER})

# Use STB
pkg_check_modules(STB REQUIRED stb)
target_link_libraries(progressia ${STB_LIBRARIES})
target_include_directories(progressia PUBLIC ${STB_INCLUDE_DIRS})
target_compile_options(progressia PUBLIC ${STB_CFLAGS_OTHER})

# Use Boost (header only)
find_package(Boost REQUIRED)
target_include_directories(progressia PUBLIC ${Boost_INCLUDE_DIRS})

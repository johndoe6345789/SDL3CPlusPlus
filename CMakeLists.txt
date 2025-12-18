cmake_minimum_required(VERSION 3.24)
project(SDL3VulkanCube LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

add_executable(spinning_cube src/main.cpp)
target_link_libraries(spinning_cube PRIVATE SDL3::SDL3 Vulkan::Vulkan)
target_compile_definitions(spinning_cube PRIVATE SDL_MAIN_HANDLED)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

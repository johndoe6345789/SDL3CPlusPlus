cmake_minimum_required(VERSION 3.24)
project(SDL3App LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(lua CONFIG REQUIRED)

add_executable(sdl3_app
    src/main.cpp
    src/app/sdl3_app_core.cpp
    src/app/sdl3_app_device.cpp
    src/app/sdl3_app_swapchain.cpp
    src/app/sdl3_app_pipeline.cpp
    src/app/sdl3_app_build.cpp
    src/app/sdl3_app_buffers.cpp
    src/app/sdl3_app_render.cpp
    src/script/cube_script.cpp
)
target_link_libraries(sdl3_app PRIVATE SDL3::SDL3 Vulkan::Vulkan lua::lua)
target_compile_definitions(sdl3_app PRIVATE SDL_MAIN_HANDLED)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/scripts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

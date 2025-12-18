cmake_minimum_required(VERSION 3.24)
project(SDL3App LANGUAGES CXX)
option(BUILD_SDL3_APP "Build the SDL3 Vulkan demo" ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

if(BUILD_SDL3_APP)
    find_package(Vulkan REQUIRED)
endif()
find_package(lua CONFIG REQUIRED)

if(BUILD_SDL3_APP)
    add_executable(sdl3_app
        src/main.cpp
        src/app/sdl3_app_core.cpp
        src/app/sdl3_app_device.cpp
        src/app/sdl3_app_swapchain.cpp
        src/app/sdl3_app_pipeline.cpp
        src/app/sdl3_app_build.cpp
        src/app/sdl3_app_buffers.cpp
        src/app/sdl3_app_render.cpp
        src/app/vulkan_api.cpp
        src/script/cube_script.cpp
    )
    target_include_directories(sdl3_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(sdl3_app PRIVATE sdl::sdl Vulkan::Vulkan lua::lua)
    target_compile_definitions(sdl3_app PRIVATE SDL_MAIN_HANDLED)

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/scripts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

enable_testing()

add_executable(cube_script_tests
    tests/test_cube_script.cpp
    src/script/cube_script.cpp
)
target_include_directories(cube_script_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(cube_script_tests PRIVATE lua::lua)
add_test(NAME cube_script_tests COMMAND cube_script_tests)

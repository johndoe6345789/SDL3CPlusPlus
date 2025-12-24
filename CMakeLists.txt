cmake_minimum_required(VERSION 3.24)
if(CMAKE_GENERATOR MATCHES "Ninja")
    if(DEFINED CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_GENERATOR_PLATFORM "" CACHE STRING "" FORCE)
    endif()
    if(DEFINED CMAKE_GENERATOR_TOOLSET)
        set(CMAKE_GENERATOR_TOOLSET "" CACHE STRING "" FORCE)
    endif()
endif()
project(SDL3App LANGUAGES CXX)
list(APPEND CMAKE_PROGRAM_PATH "C:/ProgramData/chocolatey/bin")
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build configuration for single-config generators" FORCE)
endif()
option(ENABLE_CLANG_TIDY "Automatically run clang-tidy on every target when configuring via CMake" OFF)

if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_EXE
        NAMES clang-tidy clang-tidy.exe
        HINTS "C:/Program Files/LLVM/bin" "C:/ProgramData/chocolatey/bin"
    )
    if(CLANG_TIDY_EXE)
        if(WIN32)
            set(CLANG_TIDY_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/clang_tidy_wrapper.bat")
            file(WRITE ${CLANG_TIDY_WRAPPER}
                "@echo off\n"
                "setlocal\n"
                "set \"CLANG_TIDY_EXE=${CLANG_TIDY_EXE}\"\n"
                "echo [clang-tidy] starting %*\n"
                "\"%CLANG_TIDY_EXE%\" %*\n"
                "set RET=%ERRORLEVEL%\n"
                "echo [clang-tidy] finished (exit %RET%)\n"
                "exit /b %RET%\n"
            )
        else()
            set(CLANG_TIDY_WRAPPER "${CMAKE_CURRENT_BINARY_DIR}/clang_tidy_wrapper.sh")
            file(WRITE ${CLANG_TIDY_WRAPPER}
                "#!/bin/sh\n"
                "echo \"[clang-tidy] starting $@\"\n"
                "\"${CLANG_TIDY_EXE}\" \"$@\"\n"
                "ret=$?\n"
                "echo \"[clang-tidy] finished (exit $ret)\"\n"
                "exit $ret\n"
            )
            execute_process(COMMAND ${CMAKE_COMMAND} -E chmod +x ${CLANG_TIDY_WRAPPER})
        endif()
        set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${CLANG_TIDY_WRAPPER})
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_WRAPPER}")
    else()
        message(WARNING "clang-tidy requested but not found on the PATH; disabling lint step")
    endif()
endif()
option(BUILD_SDL3_APP "Build the SDL3 Vulkan demo" ON)
set(SDL_VERSION "SDL3" CACHE STRING "SDL version to use: SDL3, SDL2, or sdl")
set_property(CACHE SDL_VERSION PROPERTY STRINGS "SDL3" "SDL2" "sdl")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(EXISTS "${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
    include("${CMAKE_BINARY_DIR}/conan_toolchain.cmake")
endif()

if(CMAKE_GENERATOR MATCHES "Ninja" OR CMAKE_GENERATOR MATCHES "Ninja Multi-Config")
    if(DEFINED CMAKE_GENERATOR_PLATFORM)
        set(CMAKE_GENERATOR_PLATFORM "" CACHE STRING "" FORCE)
    endif()
    if(DEFINED CMAKE_GENERATOR_TOOLSET)
        set(CMAKE_GENERATOR_TOOLSET "" CACHE STRING "" FORCE)
    endif()
endif()

if(BUILD_SDL3_APP)
    find_package(Vulkan REQUIRED)
endif()

# SDL is required for both the demo app and cube_script_tests (used by audio_player)
if(SDL_VERSION STREQUAL "SDL3")
    find_package(SDL3 CONFIG REQUIRED)
    set(SDL_TARGET SDL3::SDL3)
elseif(SDL_VERSION STREQUAL "SDL2")
    find_package(SDL2 CONFIG REQUIRED)
    set(SDL_TARGET SDL2::SDL2)
elseif(SDL_VERSION STREQUAL "sdl")
    find_package(SDL CONFIG REQUIRED)
    set(SDL_TARGET SDL::SDL)
else()
    message(FATAL_ERROR "Invalid SDL_VERSION: ${SDL_VERSION}. Must be SDL3, SDL2, or sdl")
endif()

find_package(lua CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(rapidjson CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

if(BUILD_SDL3_APP)
add_executable(sdl3_app
        src/main.cpp
        src/app/sdl3_app_core.cpp
        src/app/audio_player.cpp
        src/app/sdl3_app_device.cpp
        src/app/sdl3_app_swapchain.cpp
        src/app/sdl3_app_pipeline.cpp
        src/app/sdl3_app_build.cpp
        src/app/sdl3_app_buffers.cpp
        src/gui/gui_renderer.cpp
        src/app/sdl3_app_render.cpp
        src/app/vulkan_api.cpp
        src/script/cube_script.cpp
    )
    target_include_directories(sdl3_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(sdl3_app PRIVATE ${SDL_TARGET} Vulkan::Vulkan lua::lua CLI11::CLI11 rapidjson assimp::assimp Bullet::Bullet glm::glm Vorbis::vorbisfile Vorbis::vorbis)
    target_compile_definitions(sdl3_app PRIVATE SDL_MAIN_HANDLED)

    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/scripts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
endif()

enable_testing()

add_executable(cube_script_tests
    tests/test_cube_script.cpp
    src/script/cube_script.cpp
    src/app/audio_player.cpp
)
target_include_directories(cube_script_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(cube_script_tests PRIVATE ${SDL_TARGET} lua::lua assimp::assimp Bullet::Bullet glm::glm Vorbis::vorbisfile Vorbis::vorbis)
add_test(NAME cube_script_tests COMMAND cube_script_tests)

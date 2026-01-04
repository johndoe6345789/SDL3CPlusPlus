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
set(SDL_VERSION "SDL3" CACHE STRING "SDL version to use: SDL3 or sdl")
set_property(CACHE SDL_VERSION PROPERTY STRINGS "SDL3" "sdl")

if(BUILD_SDL3_APP AND NOT SDL_VERSION STREQUAL "SDL3")
    message(STATUS "Disabling BUILD_SDL3_APP because SDL_VERSION is set to '${SDL_VERSION}' instead of SDL3")
    set(BUILD_SDL3_APP OFF CACHE BOOL "Build the SDL3 Vulkan demo" FORCE)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enforce "no macros" culture mechanically
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wmacro-redefined
        -Wreserved-id-macro
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    add_compile_options(
        -Wbuiltin-macro-redefined
    )
endif()
if(MSVC)
    # MSVC equivalent warnings
    add_compile_options(/we4005)  # macro redefinition
endif()

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
elseif(SDL_VERSION STREQUAL "sdl")
    find_package(SDL CONFIG REQUIRED)
    set(SDL_TARGET SDL::SDL)
else()
    message(FATAL_ERROR "Invalid SDL_VERSION: ${SDL_VERSION}. Must be SDL3 or sdl")
endif()

find_package(lua CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

if(BUILD_SDL3_APP)
    add_executable(sdl3_app
            src/main.cpp
            src/logging/string_utils.cpp
            src/core/platform.cpp
            src/di/service_registry.cpp
            src/events/event_bus.cpp
            src/services/impl/json_config_service.cpp
            src/services/impl/logger_service.cpp
            src/services/impl/sdl_window_service.cpp
            src/services/impl/sdl_input_service.cpp
            src/services/impl/vulkan_device_service.cpp
            src/services/impl/swapchain_service.cpp
            src/services/impl/pipeline_service.cpp
            src/services/impl/buffer_service.cpp
            src/services/impl/render_command_service.cpp
            src/services/impl/sdl_audio_service.cpp
            src/services/impl/crash_recovery_service.cpp
            src/services/impl/vulkan_gui_service.cpp
            src/services/impl/bullet_physics_service.cpp
            src/services/impl/scene_service.cpp
            src/services/impl/graphics_service.cpp
            src/services/impl/lua_script_service.cpp
            src/controllers/application_controller.cpp
            src/controllers/render_controller.cpp
            src/controllers/lifecycle_controller.cpp
            src/app/service_based_app.cpp
            src/gui/gui_renderer.cpp
            src/core/vulkan_utils.cpp
            src/script/script_engine.cpp
            src/script/physics_bridge.cpp
            src/script/lua_helpers.cpp
            src/script/lua_bindings.cpp
            src/script/mesh_loader.cpp
            src/script/scene_manager.cpp
            src/script/shader_manager.cpp
            src/script/gui_manager.cpp
            src/script/audio_manager.cpp
        )
    target_include_directories(sdl3_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(sdl3_app PRIVATE ${SDL_TARGET} Vulkan::Vulkan lua::lua CLI11::CLI11 rapidjson assimp::assimp Bullet::Bullet glm::glm Vorbis::vorbisfile Vorbis::vorbis)
    target_compile_definitions(sdl3_app PRIVATE SDL_MAIN_HANDLED)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/scripts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()

enable_testing()

add_executable(script_engine_tests
    tests/test_cube_script.cpp
    src/logging/string_utils.cpp
    src/core/platform.cpp
    src/script/script_engine.cpp
    src/script/physics_bridge.cpp
    src/script/scene_manager.cpp
    src/script/shader_manager.cpp
    src/script/gui_manager.cpp
    src/script/audio_manager.cpp
    src/script/lua_helpers.cpp
    src/script/lua_bindings.cpp
    src/script/mesh_loader.cpp
)
target_include_directories(script_engine_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(script_engine_tests PRIVATE ${SDL_TARGET} lua::lua assimp::assimp Bullet::Bullet glm::glm Vorbis::vorbisfile Vorbis::vorbis)
add_test(NAME script_engine_tests COMMAND script_engine_tests)

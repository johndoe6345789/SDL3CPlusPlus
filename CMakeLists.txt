cmake_minimum_required(VERSION 3.24)
option(ENABLE_VITA "Enable Vita SDK integration" OFF)
if(ENABLE_VITA)
    if(NOT DEFINED ENV{VITASDK})
        message(FATAL_ERROR "VITASDK environment variable must be set when ENABLE_VITA is ON")
    endif()
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE FILEPATH "Vita SDK toolchain" FORCE)
    include_directories("$ENV{VITASDK}/arm-vita-eabi/include")
    link_directories("$ENV{VITASDK}/arm-vita-eabi/lib")
    # Disable SDL3 app when building for Vita
    set(BUILD_SDL3_APP OFF CACHE BOOL "Build the SDL3 bgfx demo" FORCE)
    set(BUILD_SCRIPT_TESTS OFF CACHE BOOL "Build script engine tests" FORCE)
endif()
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
if(ENABLE_VITA)
    include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
    set(VITA_APP_NAME "SDL3App")
    set(VITA_TITLEID "ABCD00001")
endif()
option(BUILD_SDL3_APP "Build the SDL3 bgfx demo" ON)
set(SDL_VERSION "SDL3" CACHE STRING "SDL version to use: SDL3 or sdl")
set_property(CACHE SDL_VERSION PROPERTY STRINGS "SDL3" "sdl")

if(BUILD_SDL3_APP AND NOT SDL_VERSION STREQUAL "SDL3")
    message(STATUS "Disabling BUILD_SDL3_APP because SDL_VERSION is set to '${SDL_VERSION}' instead of SDL3")
    set(BUILD_SDL3_APP OFF CACHE BOOL "Build the SDL3 bgfx demo" FORCE)
endif()

if(ENABLE_VITA)
    set(CMAKE_CXX_STANDARD 17)
else()
    set(CMAKE_CXX_STANDARD 23)
endif()
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

if(BUILD_SDL3_APP AND NOT ENABLE_VITA)
    find_package(shaderc CONFIG REQUIRED)
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

if(NOT ENABLE_VITA)
find_package(lua CONFIG REQUIRED)
find_package(CLI11 CONFIG REQUIRED)
find_package(RapidJSON CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(bgfx CONFIG REQUIRED)
find_package(MaterialX CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(cpptrace REQUIRED)

set(SDL3CPP_RENDER_STACK_LIBS EnTT::EnTT)
if(TARGET bgfx::bgfx)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS bgfx::bgfx)
endif()
if(TARGET bx::bx)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS bx::bx)
endif()
if(TARGET bimg::bimg)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS bimg::bimg)
endif()

set(SDL3CPP_MATERIALX_LIBS)
if(TARGET MaterialX::MaterialX)
    list(APPEND SDL3CPP_MATERIALX_LIBS MaterialX::MaterialX)
else()
foreach(candidate IN ITEMS
        MaterialX::MaterialXCore
        MaterialX::MaterialXFormat
        MaterialX::MaterialXGenShader
        MaterialX::MaterialXGenGlsl
        MaterialX::MaterialXRender)
        if(TARGET ${candidate})
            list(APPEND SDL3CPP_MATERIALX_LIBS ${candidate})
        endif()
    endforeach()
endif()
if(SDL3CPP_MATERIALX_LIBS)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS ${SDL3CPP_MATERIALX_LIBS})
endif()

set(SDL3CPP_FREETYPE_LIBS)
if(TARGET Freetype::Freetype)
    list(APPEND SDL3CPP_FREETYPE_LIBS Freetype::Freetype)
elseif(TARGET freetype::freetype)
    list(APPEND SDL3CPP_FREETYPE_LIBS freetype::freetype)
endif()
endif()

if(BUILD_SDL3_APP)
    add_executable(sdl3_app
            src/main.cpp
            src/di/service_registry.cpp
            src/events/event_bus.cpp
            src/services/impl/json_config_service.cpp
            src/services/impl/command_line_service.cpp
            src/services/impl/json_config_writer_service.cpp
            src/services/impl/logger_service.cpp
            src/services/impl/ecs_service.cpp
            src/services/impl/platform_service.cpp
            src/services/impl/script_engine_service.cpp
            src/services/impl/lua_helpers.cpp
            src/services/impl/scene_script_service.cpp
            src/services/impl/shader_script_service.cpp
            src/services/impl/materialx_shader_generator.cpp
            src/services/impl/gui_script_service.cpp
            src/services/impl/audio_command_service.cpp
            src/services/impl/physics_bridge_service.cpp
            src/services/impl/mesh_service.cpp
            src/services/impl/sdl_window_service.cpp
            src/services/impl/sdl_input_service.cpp
            src/services/impl/sdl_audio_service.cpp
            src/services/impl/crash_recovery_service.cpp
            src/services/impl/lifecycle_service.cpp
            src/services/impl/application_loop_service.cpp
            src/services/impl/render_coordinator_service.cpp
            src/services/impl/null_gui_service.cpp
            src/services/impl/bullet_physics_service.cpp
            src/services/impl/scene_service.cpp
            src/services/impl/graphics_service.cpp
            $<$<NOT:$<BOOL:${ENABLE_VITA}>>:src/services/impl/bgfx_graphics_backend.cpp>
            $<$<BOOL:${ENABLE_VITA}>:src/services/impl/gxm_graphics_backend.cpp>
            src/app/service_based_app.cpp
        )
    target_include_directories(sdl3_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(sdl3_app PRIVATE
        ${SDL_TARGET}
        lua::lua
        CLI11::CLI11
        rapidjson
        ${SDL3CPP_RENDER_STACK_LIBS}
        ${SDL3CPP_FREETYPE_LIBS}
        assimp::assimp
        Bullet::Bullet
        glm::glm
        Vorbis::vorbisfile
        Vorbis::vorbis
        cpptrace::cpptrace
    )
    if(NOT ENABLE_VITA)
        if(TARGET shaderc::shaderc)
            target_link_libraries(sdl3_app PRIVATE shaderc::shaderc)
        elseif(TARGET shaderc::shaderc_combined)
            target_link_libraries(sdl3_app PRIVATE shaderc::shaderc_combined)
        else()
            message(FATAL_ERROR "shaderc CMake target not found")
        endif()
    endif()
    target_compile_definitions(sdl3_app PRIVATE SDL_MAIN_HANDLED)

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/scripts")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/scripts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
        if(ENABLE_VITA)
            # For Vita builds, copy only Vita-specific configs            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_system_config.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_gui_runtime.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_cube_runtime.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_soundboard_runtime.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
        else()
            # For desktop builds, copy all configs
            file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
        endif()
    endif()
endif()

# Copy config files for all builds (including Vita test builds)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/config")
    if(ENABLE_VITA)
        # For Vita builds, copy only Vita-specific configs
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_system_config.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_gui_runtime.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_cube_runtime.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config/vita_soundboard_runtime.json" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/config")
    else()
        # For desktop builds, copy all configs
        file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/config" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    endif()
endif()

enable_testing()

if(NOT ENABLE_VITA)
add_executable(script_engine_tests
    tests/test_cube_script.cpp
    src/services/impl/logger_service.cpp
    src/services/impl/script_engine_service.cpp
    src/services/impl/lua_helpers.cpp
    src/services/impl/scene_script_service.cpp
    src/services/impl/shader_script_service.cpp
    src/services/impl/materialx_shader_generator.cpp
)
target_include_directories(script_engine_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(script_engine_tests PRIVATE
    ${SDL_TARGET}
    lua::lua
    ${SDL3CPP_RENDER_STACK_LIBS}
    assimp::assimp
    Bullet::Bullet
    glm::glm
    Vorbis::vorbisfile
    Vorbis::vorbis
)
add_test(NAME script_engine_tests COMMAND script_engine_tests)
endif()

add_executable(gxm_backend_tests
    tests/test_gxm_backend.cpp
    src/services/impl/gxm_graphics_backend.cpp
)
if(ENABLE_VITA)
    target_include_directories(gxm_backend_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(gxm_backend_tests PRIVATE
        SceGxm_stub
        SceDisplay_stub
        SceSysmem_stub
        SceLibKernel_stub
    )
else()
    target_include_directories(gxm_backend_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(gxm_backend_tests PRIVATE
        ${SDL_TARGET}
    )
endif()
add_test(NAME gxm_backend_tests COMMAND gxm_backend_tests)

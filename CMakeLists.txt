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
find_package(spirv-cross CONFIG REQUIRED)
find_package(materialx CONFIG REQUIRED)
find_package(Freetype CONFIG REQUIRED)
find_package(lunasvg CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(Vorbis CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(cpptrace REQUIRED)
find_package(stb CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

set(SDL3CPP_RENDER_STACK_LIBS EnTT::EnTT)
if(TARGET bgfx::bgfx)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS bgfx::bgfx)
endif()
if(TARGET bgfx::bx)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS bgfx::bx)
endif()
if(TARGET bimg::bimg)
    list(APPEND SDL3CPP_RENDER_STACK_LIBS bimg::bimg)
endif()

set(SDL3CPP_MATERIALX_LIBS)
if(TARGET materialx::materialx)
    list(APPEND SDL3CPP_MATERIALX_LIBS materialx::materialx)
else()
    foreach(candidate IN ITEMS
            materialx::MaterialXCore
            materialx::MaterialXFormat
            materialx::MaterialXGenShader
            materialx::MaterialXGenGlsl
            materialx::MaterialXRender)
        if(TARGET ${candidate})
            list(APPEND SDL3CPP_MATERIALX_LIBS ${candidate})
        endif()
    endforeach()
endif()
if(SDL3CPP_MATERIALX_LIBS)
    message(TRACE "MaterialX targets selected: ${SDL3CPP_MATERIALX_LIBS}")
    list(APPEND SDL3CPP_RENDER_STACK_LIBS ${SDL3CPP_MATERIALX_LIBS})
endif()

set(SDL3CPP_FREETYPE_LIBS)
if(TARGET Freetype::Freetype)
    list(APPEND SDL3CPP_FREETYPE_LIBS Freetype::Freetype)
elseif(TARGET freetype::freetype)
    list(APPEND SDL3CPP_FREETYPE_LIBS freetype::freetype)
endif()

set(SDL3CPP_ZIP_LIBS)
if(TARGET libzip::zip)
    list(APPEND SDL3CPP_ZIP_LIBS libzip::zip)
elseif(TARGET libzip::libzip)
    list(APPEND SDL3CPP_ZIP_LIBS libzip::libzip)
endif()

set(SDL3CPP_STB_LIBS)
if(TARGET stb::stb_image)
    list(APPEND SDL3CPP_STB_LIBS stb::stb_image)
elseif(TARGET stb::stb)
    list(APPEND SDL3CPP_STB_LIBS stb::stb)
elseif(TARGET stb)
    list(APPEND SDL3CPP_STB_LIBS stb)
endif()
endif()

## Local build of bgfx_tools shaderc sources as a library so we can link in-process.
add_library(shaderc_local STATIC
    src/bgfx_tools/shaderc/shaderc_hlsl.cpp
    src/bgfx_tools/shaderc/shaderc_metal.cpp
    src/bgfx_tools/shaderc/shaderc_mem.cpp
    src/bgfx_tools/shaderc/shaderc_spirv.cpp
    src/bgfx_tools/shaderc/shaderc_utils.cpp
)
target_include_directories(shaderc_local PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_deps/glslang"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_deps/glslang/Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_deps/glslang/glslang/Public")
target_include_directories(shaderc_local PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src")
if(TARGET bgfx::bx)
    target_link_libraries(shaderc_local PUBLIC bgfx::bx)
endif()
if(TARGET bgfx::bgfx)
    target_link_libraries(shaderc_local PUBLIC bgfx::bgfx)
endif()
if(TARGET spirv-cross::spirv-cross)
    target_link_libraries(shaderc_local PUBLIC spirv-cross::spirv-cross)
    target_include_directories(shaderc_local BEFORE PRIVATE
        $<TARGET_PROPERTY:spirv-cross::spirv-cross,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(shaderc_local PRIVATE
        $<TARGET_PROPERTY:spirv-cross::spirv-cross,INTERFACE_COMPILE_DEFINITIONS>)
    target_compile_options(shaderc_local PRIVATE
        $<TARGET_PROPERTY:spirv-cross::spirv-cross,INTERFACE_COMPILE_OPTIONS>)
endif()
# Link shaderc dependencies
if(TARGET glslang::glslang)
    target_link_libraries(shaderc_local PUBLIC glslang::glslang)
    target_include_directories(shaderc_local PRIVATE
        $<TARGET_PROPERTY:glslang::glslang,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(shaderc_local PRIVATE
        $<TARGET_PROPERTY:glslang::glslang,INTERFACE_COMPILE_DEFINITIONS>)
    target_compile_options(shaderc_local PRIVATE
        $<TARGET_PROPERTY:glslang::glslang,INTERFACE_COMPILE_OPTIONS>)
endif()
if(TARGET spirv-tools::spirv-tools)
    target_link_libraries(shaderc_local PUBLIC spirv-tools::spirv-tools)
    target_include_directories(shaderc_local PRIVATE
        $<TARGET_PROPERTY:spirv-tools::spirv-tools,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(shaderc_local PRIVATE
        $<TARGET_PROPERTY:spirv-tools::spirv-tools,INTERFACE_COMPILE_DEFINITIONS>)
    target_compile_options(shaderc_local PRIVATE
        $<TARGET_PROPERTY:spirv-tools::spirv-tools,INTERFACE_COMPILE_OPTIONS>)
endif()
if(TARGET shaderc::shaderc)
    target_link_libraries(shaderc_local PUBLIC shaderc::shaderc)
endif()

## Build geometryc tool
# add_executable(geometryc
#     src/bgfx_tools/geometryc/geometryc.cpp
# )
# target_include_directories(geometryc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
# target_link_libraries(geometryc PRIVATE ${SDL3CPP_RENDER_STACK_LIBS} meshoptimizer::meshoptimizer cgltf::cgltf)

## Build texturev tool
# add_executable(texturev
#     src/bgfx_tools/texturev/texturev.cpp
# )
# target_include_directories(texturev PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
# target_link_libraries(texturev PRIVATE ${SDL3CPP_RENDER_STACK_LIBS})

if(BUILD_SDL3_APP)
    add_executable(sdl3_app
            src/main.cpp
            src/stb_image.cpp
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
            src/services/impl/shader_pipeline_validator.cpp
            src/services/impl/gui_script_service.cpp
            $<$<NOT:$<BOOL:${ENABLE_VITA}>>:src/services/impl/bgfx_gui_service.cpp>
            $<$<NOT:$<BOOL:${ENABLE_VITA}>>:src/services/impl/bgfx_shader_compiler.cpp>
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
            src/services/impl/bgfx_gui_service.cpp
            src/services/impl/bgfx_shader_compiler.cpp
            src/services/impl/pipeline_compiler_service.cpp
            src/services/impl/bullet_physics_service.cpp
            src/services/impl/scene_service.cpp
            src/services/impl/graphics_service.cpp
            $<$<NOT:$<BOOL:${ENABLE_VITA}>>:src/services/impl/bgfx_graphics_backend.cpp>
            $<$<BOOL:${ENABLE_VITA}>:src/services/impl/gxm_graphics_backend.cpp>
            src/app/service_based_app.cpp
        )
    target_include_directories(sdl3_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_include_directories(sdl3_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc")
    target_link_libraries(sdl3_app PRIVATE
        ${SDL_TARGET}
        lua::lua
        CLI11::CLI11
        rapidjson
        ${SDL3CPP_STB_LIBS}
        ${SDL3CPP_RENDER_STACK_LIBS}
        ${SDL3CPP_FREETYPE_LIBS}
        $<$<NOT:$<BOOL:${ENABLE_VITA}>>:lunasvg::lunasvg>
        ${SDL3CPP_ZIP_LIBS}
        assimp::assimp
        Bullet::Bullet
        glm::glm
        Vorbis::vorbisfile
        Vorbis::vorbis
        lunasvg::lunasvg
        cpptrace::cpptrace
    )
    if(NOT ENABLE_VITA)
        if(TARGET shaderc_local)
            target_link_libraries(sdl3_app PRIVATE shaderc_local)
        else()
            message(WARNING "shaderc_local target not found; in-process shader compilation will be unavailable.")
        endif()
        if(TARGET shaderc::shaderc)
            target_link_libraries(sdl3_app PRIVATE shaderc::shaderc)
        elseif(TARGET shaderc::shaderc_combined)
            target_link_libraries(sdl3_app PRIVATE shaderc::shaderc_combined)
        else()
            message(WARNING "shaderc CMake target not found; PipelineCompilerService will be unavailable.")
        endif()
    endif()
    target_compile_definitions(sdl3_app PRIVATE SDL_MAIN_HANDLED
        $<$<BOOL:${ENABLE_VITA}>:SDL3CPP_ENABLE_VITA>)

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
    src/services/impl/bgfx_graphics_backend.cpp
    src/services/impl/bgfx_shader_compiler.cpp
    src/services/impl/logger_service.cpp
    src/services/impl/mesh_service.cpp
    src/services/impl/physics_bridge_service.cpp
    src/services/impl/platform_service.cpp
    src/services/impl/script_engine_service.cpp
    src/services/impl/lua_helpers.cpp
    src/services/impl/scene_script_service.cpp
    src/services/impl/shader_script_service.cpp
    src/services/impl/materialx_shader_generator.cpp
    src/services/impl/shader_pipeline_validator.cpp
    src/services/impl/sdl_window_service.cpp
    src/services/impl/ecs_service.cpp
    src/services/impl/scene_service.cpp
    src/events/event_bus.cpp
    src/stb_image.cpp
    src/services/impl/pipeline_compiler_service.cpp
)
target_include_directories(script_engine_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(script_engine_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc")
target_link_libraries(script_engine_tests PRIVATE
    ${SDL_TARGET}
    lua::lua
    ${SDL3CPP_RENDER_STACK_LIBS}
    assimp::assimp
    Bullet::Bullet
    glm::glm
    Vorbis::vorbisfile
    Vorbis::vorbis
    zip::zip
    libzip::zip
)
if(TARGET shaderc_local)
    target_link_libraries(script_engine_tests PRIVATE shaderc_local)
elseif(TARGET shaderc::shaderc)
elseif(TARGET shaderc::shaderc_combined)
    target_link_libraries(script_engine_tests PRIVATE shaderc::shaderc_combined)
else()
    if(TARGET shaderc_local)
        target_link_libraries(script_engine_tests PRIVATE shaderc_local)
    else()
        message(WARNING "shaderc CMake target not found; skipping link for script_engine_tests.")
    endif()
endif()
add_test(NAME script_engine_tests COMMAND script_engine_tests)

add_executable(bgfx_gui_service_tests
    tests/test_bgfx_gui_service.cpp
    src/services/impl/bgfx_gui_service.cpp
    src/services/impl/bgfx_shader_compiler.cpp
    src/services/impl/materialx_shader_generator.cpp
    src/services/impl/shader_pipeline_validator.cpp
    src/services/impl/pipeline_compiler_service.cpp
)
target_include_directories(bgfx_gui_service_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(bgfx_gui_service_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc")
target_link_libraries(bgfx_gui_service_tests PRIVATE
    ${SDL_TARGET}
    ${SDL3CPP_RENDER_STACK_LIBS}
    ${SDL3CPP_FREETYPE_LIBS}
    lunasvg::lunasvg
)
if(TARGET shaderc_local)
    target_link_libraries(bgfx_gui_service_tests PRIVATE shaderc_local)
elseif(TARGET shaderc::shaderc)
elseif(TARGET shaderc::shaderc_combined)
    target_link_libraries(bgfx_gui_service_tests PRIVATE shaderc::shaderc_combined)
else()
    if(TARGET shaderc_local)
        target_link_libraries(bgfx_gui_service_tests PRIVATE shaderc_local)
    else()
        message(WARNING "shaderc CMake target not found; skipping link for bgfx_gui_service_tests.")
    endif()
endif()
add_test(NAME bgfx_gui_service_tests COMMAND bgfx_gui_service_tests)

# Test: Vulkan shader linking (TDD test for walls/ceiling/floor rendering)
add_executable(vulkan_shader_linking_test
    tests/test_vulkan_shader_linking.cpp
    src/services/impl/bgfx_graphics_backend.cpp
    src/services/impl/bgfx_gui_service.cpp
    src/services/impl/bgfx_shader_compiler.cpp
    src/services/impl/json_config_service.cpp
    src/services/impl/materialx_shader_generator.cpp
    src/services/impl/shader_pipeline_validator.cpp
    src/services/impl/platform_service.cpp
    src/services/impl/sdl_window_service.cpp
    src/events/event_bus.cpp
    src/stb_image.cpp
    src/services/impl/pipeline_compiler_service.cpp
)
target_include_directories(vulkan_shader_linking_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(vulkan_shader_linking_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc")
target_link_libraries(vulkan_shader_linking_test PRIVATE
    ${SDL_TARGET}
    ${SDL3CPP_RENDER_STACK_LIBS}
    ${SDL3CPP_FREETYPE_LIBS}
    ${SDL3CPP_STB_LIBS}
    lunasvg::lunasvg
    rapidjson
    glm::glm
)
if(TARGET shaderc_local)
    target_link_libraries(vulkan_shader_linking_test PRIVATE shaderc_local)
elseif(TARGET shaderc::shaderc)
    target_link_libraries(vulkan_shader_linking_test PRIVATE shaderc::shaderc)
elseif(TARGET shaderc::shaderc_combined)
    target_link_libraries(vulkan_shader_linking_test PRIVATE shaderc::shaderc_combined)
else()
    message(WARNING "shaderc CMake target not found; skipping link for vulkan_shader_linking_test.")
endif()
add_test(NAME vulkan_shader_linking_test COMMAND vulkan_shader_linking_test)

# Test: Shader Pipeline Validator (mega-strict validation system)
add_executable(shader_pipeline_validator_test
    tests/shader_pipeline_validator_test.cpp
    src/services/impl/shader_pipeline_validator.cpp
)
target_include_directories(shader_pipeline_validator_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(shader_pipeline_validator_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME shader_pipeline_validator_test COMMAND shader_pipeline_validator_test)

# Test: Shaderc Uniform Mapping (guards Vulkan uniform metadata correctness)
add_executable(shaderc_uniform_mapping_test
    tests/shaderc_uniform_mapping_test.cpp
)
target_include_directories(shaderc_uniform_mapping_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc")
target_link_libraries(shaderc_uniform_mapping_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    shaderc_local
)
add_test(NAME shaderc_uniform_mapping_test COMMAND shaderc_uniform_mapping_test)

# Test: MaterialX Shader Generator Integration (validates shader generation + validation together)
add_executable(materialx_shader_generator_integration_test
    tests/materialx_shader_generator_integration_test.cpp
    src/services/impl/materialx_shader_generator.cpp
    src/services/impl/shader_pipeline_validator.cpp
)
target_include_directories(materialx_shader_generator_integration_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(materialx_shader_generator_integration_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${SDL3CPP_MATERIALX_LIBS}
)
add_test(NAME materialx_shader_generator_integration_test COMMAND materialx_shader_generator_integration_test)

# Test: Bgfx Texture Loading (investigates texture loading crash)
add_executable(bgfx_texture_loading_test
    tests/bgfx_texture_loading_test.cpp
)
target_include_directories(bgfx_texture_loading_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(bgfx_texture_loading_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME bgfx_texture_loading_test COMMAND bgfx_texture_loading_test)

# Test: Bgfx Initialization Order (documents timing requirements and crash fix)
add_executable(bgfx_initialization_order_test
    tests/bgfx_initialization_order_test.cpp
)
target_include_directories(bgfx_initialization_order_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(bgfx_initialization_order_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME bgfx_initialization_order_test COMMAND bgfx_initialization_order_test)

# Test: Bgfx Frame Requirement (TDD test - validates the actual bgfx behavior)
add_executable(bgfx_frame_requirement_test
    tests/bgfx_frame_requirement_test.cpp
)
target_include_directories(bgfx_frame_requirement_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(bgfx_frame_requirement_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${SDL3CPP_RENDER_STACK_LIBS}
    SDL3::SDL3
)
add_test(NAME bgfx_frame_requirement_test COMMAND bgfx_frame_requirement_test)

# Test: Render coordinator initialization order (TDD guard for shader load timing)
add_executable(render_coordinator_init_order_test
    tests/render_coordinator_init_order_test.cpp
    src/services/impl/render_coordinator_service.cpp
)
target_include_directories(render_coordinator_init_order_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(render_coordinator_init_order_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME render_coordinator_init_order_test COMMAND render_coordinator_init_order_test)

# Test: Bgfx backend frame guard (ensures texture loads are blocked before first frame)
add_executable(bgfx_backend_frame_guard_test
    tests/bgfx_backend_frame_guard_test.cpp
    src/services/impl/bgfx_graphics_backend.cpp
    src/services/impl/bgfx_shader_compiler.cpp
    src/stb_image.cpp
)
target_include_directories(bgfx_backend_frame_guard_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_include_directories(bgfx_backend_frame_guard_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src/bgfx_tools/shaderc")
target_link_libraries(bgfx_backend_frame_guard_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    ${SDL3CPP_RENDER_STACK_LIBS}
    ${SDL3CPP_STB_LIBS}
    glm::glm
    SDL3::SDL3
)
if(TARGET shaderc_local)
    target_link_libraries(bgfx_backend_frame_guard_test PRIVATE shaderc_local)
elseif(TARGET shaderc::shaderc)
    target_link_libraries(bgfx_backend_frame_guard_test PRIVATE shaderc::shaderc)
elseif(TARGET shaderc::shaderc_combined)
    target_link_libraries(bgfx_backend_frame_guard_test PRIVATE shaderc::shaderc_combined)
else()
    message(WARNING "shaderc CMake target not found; skipping link for bgfx_backend_frame_guard_test.")
endif()
add_test(NAME bgfx_backend_frame_guard_test COMMAND bgfx_backend_frame_guard_test)

# Test: Gui script service missing fields (logs trace and uses defaults)
add_executable(gui_script_service_missing_fields_test
    tests/gui_script_service_missing_fields_test.cpp
    src/services/impl/gui_script_service.cpp
    src/services/impl/lua_helpers.cpp
)
target_include_directories(gui_script_service_missing_fields_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gui_script_service_missing_fields_test PRIVATE
    GTest::gtest
    GTest::gtest_main
    lua::lua
    glm::glm
)
add_test(NAME gui_script_service_missing_fields_test COMMAND gui_script_service_missing_fields_test)

# Test: Graphics service buffer lifecycle (TDD guard for VRAM leaks on reupload)
add_executable(graphics_service_buffer_lifecycle_test
    tests/graphics_service_buffer_lifecycle_test.cpp
    src/services/impl/graphics_service.cpp
)
target_include_directories(graphics_service_buffer_lifecycle_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(graphics_service_buffer_lifecycle_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME graphics_service_buffer_lifecycle_test COMMAND graphics_service_buffer_lifecycle_test)

# Test: Bgfx Draw bounds validation (TDD test for buffer overflow crash)
add_executable(bgfx_draw_bounds_validation_test
    tests/bgfx_draw_bounds_validation_test.cpp
)
target_include_directories(bgfx_draw_bounds_validation_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(bgfx_draw_bounds_validation_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME bgfx_draw_bounds_validation_test COMMAND bgfx_draw_bounds_validation_test)

# Test: GUI shader linking failure (TDD test for int→sampler mapping bug)
add_executable(gui_shader_linking_failure_test
    tests/gui_shader_linking_failure_test.cpp
)
target_include_directories(gui_shader_linking_failure_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(gui_shader_linking_failure_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME gui_shader_linking_failure_test COMMAND gui_shader_linking_failure_test)

# Test: Bgfx Draw bounds crash test (REAL TDD - tests should be DISABLED until fix implemented)
add_executable(bgfx_draw_bounds_crash_test
    tests/bgfx_draw_bounds_crash_test.cpp
)
target_include_directories(bgfx_draw_bounds_crash_test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(bgfx_draw_bounds_crash_test PRIVATE
    GTest::gtest
    GTest::gtest_main
)
add_test(NAME bgfx_draw_bounds_crash_test COMMAND bgfx_draw_bounds_crash_test)
endif()

if(ENABLE_VITA)
    add_executable(gxm_backend_tests
        tests/test_gxm_backend.cpp
        src/services/impl/gxm_graphics_backend.cpp
    )
    target_include_directories(gxm_backend_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(gxm_backend_tests PRIVATE
        SceGxm_stub
        SceDisplay_stub
        SceSysmem_stub
        SceLibKernel_stub
    )
    add_test(NAME gxm_backend_tests COMMAND gxm_backend_tests)
endif()

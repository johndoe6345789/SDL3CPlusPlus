cmake_minimum_required(VERSION 3.20)
project(bgfx_experiment)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use parent gameengine's Conan-generated package configs
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../build/Release/generators")

# Find packages (from Conan)
# Note: bx and bimg are components of bgfx, not separate packages
find_package(bgfx CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)

# Simple Triangle - standalone colored triangle example
add_executable(simple_triangle simple_triangle.cpp)
target_link_libraries(simple_triangle PRIVATE
    bgfx::bgfx
    bgfx::bx
    bgfx::bimg
    SDL3::SDL3
)

# Standalone Cubes - 121 rotating colored cubes (11x11 grid)
add_executable(standalone_cubes standalone_cubes.cpp)
target_link_libraries(standalone_cubes PRIVATE
    bgfx::bgfx
    bgfx::bx
    bgfx::bimg
    SDL3::SDL3
)

# Copy shader files to build directory
add_custom_command(TARGET standalone_cubes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vs_cubes.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_cubes.bin
        $<TARGET_FILE_DIR:standalone_cubes>
    COMMENT "Copying Metal shader binaries"
)

# Code sign on macOS for Metal access
if(APPLE)
    add_custom_command(TARGET standalone_cubes POST_BUILD
        COMMAND codesign --force --sign - --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/metal.entitlements $<TARGET_FILE:standalone_cubes>
        COMMENT "Code signing standalone_cubes with Metal entitlements"
    )
endif()

# Pure SDL3 Cubes - no bgfx, direct rendering with SDL_RenderGeometry
add_executable(sdl3_cubes sdl3_cubes.cpp)
target_link_libraries(sdl3_cubes PRIVATE
    SDL3::SDL3
)

message(STATUS "bgfx experiment configured")
message(STATUS "  simple_triangle: Standalone colored triangle")
message(STATUS "  standalone_cubes: 121 rotating cubes in 11x11 grid (bgfx/Noop)")
message(STATUS "  sdl3_cubes: 121 rotating cubes (pure SDL3, ACTUAL RENDERING)")
message(STATUS "  Build with: cmake -B build && cmake --build build")
message(STATUS "  Run with: ./build/sdl3_cubes")

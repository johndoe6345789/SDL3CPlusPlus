cmake_minimum_required(VERSION 3.20)
project(standalone_workflow_cubes)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Use parent gameengine's Conan-generated package configs
set(CMAKE_PREFIX_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../build/Release/generators")

# Find packages (from Conan)
find_package(bgfx CONFIG REQUIRED)
find_package(SDL3 CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

# Create executable
add_executable(standalone_workflow_cubes main.cpp)

# Link libraries
target_link_libraries(standalone_workflow_cubes PRIVATE
    bgfx::bgfx
    bgfx::bx
    bgfx::bimg
    SDL3::SDL3
    nlohmann_json::nlohmann_json
)

# Copy shader files to build directory
add_custom_command(TARGET standalone_workflow_cubes POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/vs_cubes.bin
        ${CMAKE_CURRENT_SOURCE_DIR}/fs_cubes.bin
        $<TARGET_FILE_DIR:standalone_workflow_cubes>
    COMMENT "Copying Metal shader binaries"
)

# Code sign on macOS for Metal access
if(APPLE)
    add_custom_command(TARGET standalone_workflow_cubes POST_BUILD
        COMMAND codesign --force --sign - --entitlements ${CMAKE_CURRENT_SOURCE_DIR}/../metal.entitlements $<TARGET_FILE:standalone_workflow_cubes>
        COMMENT "Code signing standalone_workflow_cubes with Metal entitlements"
    )
endif()

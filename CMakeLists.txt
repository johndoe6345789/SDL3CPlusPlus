cmake_minimum_required(VERSION 3.24)
project(SDL3VulkanCube LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SDL3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB LUA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lua/src/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX "lua\\.c$")
list(FILTER LUA_SOURCES EXCLUDE REGEX "luac\\.c$")
add_library(lua_static STATIC ${LUA_SOURCES})
target_include_directories(lua_static PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/vendor/lua/src")
target_compile_features(lua_static PUBLIC c_std_99)

add_executable(spinning_cube
    src/main.cpp
    src/app/vulkan_cube_app.cpp
    src/script/cube_script.cpp
)
target_link_libraries(spinning_cube PRIVATE SDL3::SDL3 Vulkan::Vulkan lua_static)
target_compile_definitions(spinning_cube PRIVATE SDL_MAIN_HANDLED)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/shaders" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/scripts" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")

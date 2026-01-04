#pragma once

#include "../services/interfaces/graphics_types.hpp"
#include <lua.hpp>

#include <string>
#include <unordered_map>

namespace sdl3cpp::script {

class ShaderManager {
public:
    explicit ShaderManager(lua_State* L);

    std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LoadShaderPathsMap();

private:
    lua_State* L_;

    sdl3cpp::services::ShaderPaths ReadShaderPathsTable(int index);
    std::string GetLuaError();
};

} // namespace sdl3cpp::script
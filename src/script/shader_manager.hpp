#pragma once

#include <lua.hpp>

#include <string>
#include <unordered_map>

namespace sdl3cpp::script {

class ShaderManager {
public:
    struct ShaderPaths {
        std::string vertex;
        std::string fragment;
    };

    explicit ShaderManager(lua_State* L);

    std::unordered_map<std::string, ShaderPaths> LoadShaderPathsMap();

private:
    lua_State* L_;

    ShaderPaths ReadShaderPathsTable(int index);
    std::string GetLuaError();
};

} // namespace sdl3cpp::script
#pragma once

#include "../services/interfaces/graphics_types.hpp"
#include <lua.hpp>

#include <memory>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services {
class ILogger;
}

namespace sdl3cpp::script {

class ShaderManager {
public:
    ShaderManager(lua_State* L, std::shared_ptr<services::ILogger> logger);

    std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LoadShaderPathsMap();

private:
    lua_State* L_;
    std::shared_ptr<services::ILogger> logger_;

    sdl3cpp::services::ShaderPaths ReadShaderPathsTable(int index);
    std::string GetLuaError();
};

} // namespace sdl3cpp::script

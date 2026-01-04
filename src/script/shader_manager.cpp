#include "script/shader_manager.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sdl3cpp::script {

ShaderManager::ShaderManager(lua_State* L, std::shared_ptr<services::ILogger> logger)
    : L_(L), logger_(logger) {
    if (logger_) {
        logger_->Trace("ShaderManager", "ShaderManager");
    }
}

std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> ShaderManager::LoadShaderPathsMap() {
    if (logger_) {
        logger_->Trace("ShaderManager", "LoadShaderPathsMap");
    }
    lua_getglobal(L_, "get_shader_paths");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        if (logger_) {
            logger_->Error("Lua function 'get_shader_paths' is missing");
        }
        throw std::runtime_error("Lua function 'get_shader_paths' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = GetLuaError();
        lua_pop(L_, 1);
        if (logger_) {
            logger_->Error("Lua get_shader_paths failed: " + message);
        }
        throw std::runtime_error("Lua get_shader_paths failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        if (logger_) {
            logger_->Error("'get_shader_paths' did not return a table");
        }
        throw std::runtime_error("'get_shader_paths' did not return a table");
    }

    std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> shaderMap;
    lua_pushnil(L_);
    while (lua_next(L_, -2) != 0) {
        if (lua_isstring(L_, -2) && lua_istable(L_, -1)) {
            std::string key = lua_tostring(L_, -2);
            shaderMap.emplace(key, ReadShaderPathsTable(-1));
        }
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    if (shaderMap.empty()) {
        if (logger_) {
            logger_->Error("'get_shader_paths' did not return any shader variants");
        }
        throw std::runtime_error("'get_shader_paths' did not return any shader variants");
    }
    return shaderMap;
}

sdl3cpp::services::ShaderPaths ShaderManager::ReadShaderPathsTable(int index) {
    sdl3cpp::services::ShaderPaths paths;
    int absIndex = lua_absindex(L_, index);

    lua_getfield(L_, absIndex, "vertex");
    if (!lua_isstring(L_, -1)) {
        lua_pop(L_, 1);
        if (logger_) {
            logger_->Error("Shader path 'vertex' must be a string");
        }
        throw std::runtime_error("Shader path 'vertex' must be a string");
    }
    paths.vertex = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    lua_getfield(L_, absIndex, "fragment");
    if (!lua_isstring(L_, -1)) {
        lua_pop(L_, 1);
        if (logger_) {
            logger_->Error("Shader path 'fragment' must be a string");
        }
        throw std::runtime_error("Shader path 'fragment' must be a string");
    }
    paths.fragment = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    return paths;
}

std::string ShaderManager::GetLuaError() {
    const char* message = lua_tostring(L_, -1);
    return message ? message : "unknown lua error";
}

} // namespace sdl3cpp::script

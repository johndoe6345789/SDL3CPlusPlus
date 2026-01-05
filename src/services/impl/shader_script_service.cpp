#include "shader_script_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

namespace sdl3cpp::services::impl {

ShaderScriptService::ShaderScriptService(std::shared_ptr<IScriptEngineService> engineService,
                                         std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "ShaderScriptService",
                       "engineService=" + std::string(engineService_ ? "set" : "null"));
    }
}

std::unordered_map<std::string, ShaderPaths> ShaderScriptService::LoadShaderPathsMap() {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "LoadShaderPathsMap");
    }
    lua_State* L = GetLuaState();

    lua_getglobal(L, "get_shader_paths");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua function 'get_shader_paths' is missing");
        }
        throw std::runtime_error("Lua function 'get_shader_paths' is missing");
    }
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        std::string message = lua::GetLuaError(L);
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Lua get_shader_paths failed: " + message);
        }
        throw std::runtime_error("Lua get_shader_paths failed: " + message);
    }
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("'get_shader_paths' did not return a table");
        }
        throw std::runtime_error("'get_shader_paths' did not return a table");
    }

    std::unordered_map<std::string, ShaderPaths> shaderMap;
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_isstring(L, -2) && lua_istable(L, -1)) {
            std::string key = lua_tostring(L, -2);
            shaderMap.emplace(key, ReadShaderPathsTable(L, -1));
        }
        lua_pop(L, 1);
    }

    lua_pop(L, 1);
    if (shaderMap.empty()) {
        if (logger_) {
            logger_->Error("'get_shader_paths' did not return any shader variants");
        }
        throw std::runtime_error("'get_shader_paths' did not return any shader variants");
    }
    return shaderMap;
}

ShaderPaths ShaderScriptService::ReadShaderPathsTable(lua_State* L, int index) const {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "ReadShaderPathsTable",
                       "index=" + std::to_string(index));
    }
    ShaderPaths paths;
    int absIndex = lua_absindex(L, index);

    lua_getfield(L, absIndex, "vertex");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Shader path 'vertex' must be a string");
        }
        throw std::runtime_error("Shader path 'vertex' must be a string");
    }
    paths.vertex = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, absIndex, "fragment");
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        if (logger_) {
            logger_->Error("Shader path 'fragment' must be a string");
        }
        throw std::runtime_error("Shader path 'fragment' must be a string");
    }
    paths.fragment = lua_tostring(L, -1);
    lua_pop(L, 1);

    return paths;
}

lua_State* ShaderScriptService::GetLuaState() const {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "GetLuaState");
    }
    if (!engineService_) {
        throw std::runtime_error("Shader script service is missing script engine service");
    }
    lua_State* state = engineService_->GetLuaState();
    if (!state) {
        throw std::runtime_error("Lua state is not initialized");
    }
    return state;
}

}  // namespace sdl3cpp::services::impl

#include "shader_script_service.hpp"

#include "lua_helpers.hpp"
#include "services/interfaces/i_logger.hpp"

#include <lua.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>
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

    auto readRequiredPath = [&](const char* field, std::string& target) {
        lua_getfield(L, absIndex, field);
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Shader path '" + std::string(field) + "' must be a string");
            }
            throw std::runtime_error("Shader path '" + std::string(field) + "' must be a string");
        }
        target = lua_tostring(L, -1);
        lua_pop(L, 1);
    };

    auto readOptionalPath = [&](const char* field, std::string& target) {
        lua_getfield(L, absIndex, field);
        if (lua_isstring(L, -1)) {
            target = lua_tostring(L, -1);
        } else if (!lua_isnil(L, -1)) {
            lua_pop(L, 1);
            if (logger_) {
                logger_->Error("Shader path '" + std::string(field) + "' must be a string when provided");
            }
            throw std::runtime_error("Shader path '" + std::string(field) + "' must be a string when provided");
        }
        lua_pop(L, 1);
    };

    readRequiredPath("vertex", paths.vertex);
    readRequiredPath("fragment", paths.fragment);
    readOptionalPath("geometry", paths.geometry);
    readOptionalPath("tesc", paths.tessControl);
    readOptionalPath("tese", paths.tessEval);
    readOptionalPath("compute", paths.compute);

    auto resolveIfPresent = [&](std::string& value) {
        if (!value.empty()) {
            value = ResolveShaderPath(value);
        }
    };

    resolveIfPresent(paths.vertex);
    resolveIfPresent(paths.fragment);
    resolveIfPresent(paths.geometry);
    resolveIfPresent(paths.tessControl);
    resolveIfPresent(paths.tessEval);
    resolveIfPresent(paths.compute);

    return paths;
}

std::string ShaderScriptService::ResolveShaderPath(const std::string& path) const {
    if (path.empty()) {
        return path;
    }

    std::filesystem::path rawPath(path);
    if (rawPath.is_absolute()) {
        return rawPath.string();
    }

    std::vector<std::filesystem::path> searchRoots;
    if (engineService_) {
        auto scriptDir = engineService_->GetScriptDirectory();
        if (!scriptDir.empty()) {
            auto projectRoot = scriptDir.parent_path();
            if (!projectRoot.empty()) {
                searchRoots.push_back(projectRoot);
            }
            searchRoots.push_back(scriptDir);
        }
    }
    searchRoots.push_back(std::filesystem::current_path());

    for (const auto& root : searchRoots) {
        std::filesystem::path candidate = root / rawPath;
        if (std::filesystem::exists(candidate)) {
            if (logger_) {
                logger_->Trace("ShaderScriptService", "ResolveShaderPath",
                               "path=" + path + ", resolved=" + candidate.string());
            }
            return candidate.string();
        }
    }

    if (logger_) {
        logger_->Trace("ShaderScriptService", "ResolveShaderPath",
                       "path=" + path + ", resolved=unmodified");
    }
    return rawPath.string();
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

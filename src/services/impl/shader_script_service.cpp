#include "shader_script_service.hpp"

#include "services/interfaces/i_logger.hpp"

#include <stdexcept>
#include <string>
#include <unordered_map>

namespace sdl3cpp::services::impl {

ShaderScriptService::ShaderScriptService(std::shared_ptr<IScriptEngineService> engineService,
                                         std::shared_ptr<IShaderSystemRegistry> shaderSystemRegistry,
                                         std::shared_ptr<ILogger> logger)
    : engineService_(std::move(engineService)),
      shaderSystemRegistry_(std::move(shaderSystemRegistry)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("ShaderScriptService", "ShaderScriptService",
                       "engineService=" + std::string(engineService_ ? "set" : "null") +
                       ", shaderSystemRegistry=" + std::string(shaderSystemRegistry_ ? "set" : "null"));
    }
}

std::unordered_map<std::string, ShaderPaths> ShaderScriptService::LoadShaderPathsMap() {
    if (!shaderSystemRegistry_) {
        throw std::runtime_error("Shader script service requires a shader system registry");
    }
    return shaderSystemRegistry_->BuildShaderMap();
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

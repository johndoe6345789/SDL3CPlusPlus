#include "shader_script_service.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

ShaderScriptService::ShaderScriptService(std::shared_ptr<IScriptEngineService> engineService)
    : engineService_(std::move(engineService)) {
}

std::unordered_map<std::string, ShaderPaths> ShaderScriptService::LoadShaderPathsMap() {
    return engineService_->GetEngine().LoadShaderPathsMap();
}

}  // namespace sdl3cpp::services::impl

#include "scene_script_service.hpp"
#include <utility>

namespace sdl3cpp::services::impl {

SceneScriptService::SceneScriptService(std::shared_ptr<IScriptEngineService> engineService)
    : engineService_(std::move(engineService)) {
}

std::vector<script::SceneManager::SceneObject> SceneScriptService::LoadSceneObjects() {
    return engineService_->GetEngine().LoadSceneObjects();
}

std::array<float, 16> SceneScriptService::ComputeModelMatrix(int functionRef, float time) {
    return engineService_->GetEngine().ComputeModelMatrix(functionRef, time);
}

std::array<float, 16> SceneScriptService::GetViewProjectionMatrix(float aspect) {
    return engineService_->GetEngine().GetViewProjectionMatrix(aspect);
}

}  // namespace sdl3cpp::services::impl

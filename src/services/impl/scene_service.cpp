#include "scene_service.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

SceneService::SceneService(std::shared_ptr<ISceneScriptService> scriptService, std::shared_ptr<ILogger> logger)
    : scriptService_(scriptService), logger_(logger) {
    logger_->Trace("SceneService", "SceneService",
                   "scriptService=" + std::string(scriptService_ ? "set" : "null"));

    if (!scriptService_) {
        throw std::invalid_argument("Scene script service cannot be null");
    }
}

SceneService::~SceneService() {
    logger_->Trace("SceneService", "~SceneService");
    if (initialized_) {
        Shutdown();
    }
}

void SceneService::LoadScene(const std::vector<SceneObject>& objects) {
    logger_->Trace("SceneService", "LoadScene",
                   "objects.size=" + std::to_string(objects.size()));

    sceneObjects_ = objects;
    initialized_ = true;
}

void SceneService::UpdateScene(float deltaTime) {
    logger_->Trace("SceneService", "UpdateScene",
                   "deltaTime=" + std::to_string(deltaTime));

    // Scene updates would go here (animations, physics, etc.)
    // For now, this is a placeholder
    (void)deltaTime;
}

std::vector<RenderCommand> SceneService::GetRenderCommands(float time) const {
    logger_->Trace("SceneService", "GetRenderCommands",
                   "time=" + std::to_string(time));

    if (!initialized_) {
        return {};
    }

    std::vector<RenderCommand> commands;
    commands.reserve(sceneObjects_.size());

    for (const auto& obj : sceneObjects_) {
        RenderCommand cmd;
        cmd.indexOffset = 0;  // Assume each object has its own index buffer
        cmd.indexCount = static_cast<uint32_t>(obj.indices.size());
        cmd.vertexOffset = 0;  // Assume each object has its own vertex buffer
        cmd.shaderKey = obj.shaderKey;
        cmd.modelMatrix = scriptService_->ComputeModelMatrix(obj.computeModelMatrixRef, time);
        commands.push_back(cmd);
    }

    return commands;
}

void SceneService::Clear() {
    logger_->Trace("SceneService", "Clear");

    sceneObjects_.clear();
    initialized_ = false;
}

size_t SceneService::GetObjectCount() const {
    logger_->Trace("SceneService", "GetObjectCount");

    return sceneObjects_.size();
}

void SceneService::Shutdown() noexcept {
    logger_->Trace("SceneService", "Shutdown");

    Clear();
}

}  // namespace sdl3cpp::services::impl

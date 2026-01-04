#include "scene_service.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

SceneService::SceneService(std::shared_ptr<IScriptService> scriptService, std::shared_ptr<ILogger> logger)
    : scriptService_(scriptService), logger_(logger) {
    logger_->TraceFunction(__func__);

    if (!scriptService_) {
        throw std::invalid_argument("Script service cannot be null");
    }
}

SceneService::~SceneService() {
    logger_->TraceFunction(__func__);
    if (initialized_) {
        Shutdown();
    }
}

void SceneService::LoadScene(const std::vector<SceneObject>& objects) {
    logger_->TraceFunction(__func__);

    sceneObjects_ = objects;
    initialized_ = true;
}

void SceneService::UpdateScene(float deltaTime) {
    logger_->TraceFunction(__func__);

    // Scene updates would go here (animations, physics, etc.)
    // For now, this is a placeholder
    (void)deltaTime;
}

std::vector<RenderCommand> SceneService::GetRenderCommands(float time) const {
    logger_->TraceFunction(__func__);

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
    logger_->TraceFunction(__func__);

    sceneObjects_.clear();
    initialized_ = false;
}

size_t SceneService::GetObjectCount() const {
    logger_->TraceFunction(__func__);

    return sceneObjects_.size();
}

void SceneService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    Clear();
}

}  // namespace sdl3cpp::services::impl
#include "scene_service.hpp"
#include "../../logging/logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

SceneService::SceneService(std::shared_ptr<IScriptService> scriptService)
    : scriptService_(scriptService) {
    logging::TraceGuard trace("SceneService::SceneService");

    if (!scriptService_) {
        throw std::invalid_argument("Script service cannot be null");
    }
}

SceneService::~SceneService() {
    logging::TraceGuard trace("SceneService::~SceneService");
    if (initialized_) {
        Shutdown();
    }
}

void SceneService::LoadScene(const std::vector<SceneObject>& objects) {
    logging::TraceGuard trace("SceneService::LoadScene");

    sceneObjects_ = objects;
    initialized_ = true;
}

void SceneService::UpdateScene(float deltaTime) {
    logging::TraceGuard trace("SceneService::UpdateScene");

    // Scene updates would go here (animations, physics, etc.)
    // For now, this is a placeholder
    (void)deltaTime;
}

std::vector<RenderCommand> SceneService::GetRenderCommands(float time) const {
    logging::TraceGuard trace("SceneService::GetRenderCommands");

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
    logging::TraceGuard trace("SceneService::Clear");

    sceneObjects_.clear();
    initialized_ = false;
}

size_t SceneService::GetObjectCount() const {
    logging::TraceGuard trace("SceneService::GetObjectCount");

    return sceneObjects_.size();
}

void SceneService::Shutdown() noexcept {
    logging::TraceGuard trace("SceneService::Shutdown");

    Clear();
}

}  // namespace sdl3cpp::services::impl
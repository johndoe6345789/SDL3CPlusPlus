#include "services/interfaces/workflow/scene/workflow_scene_clear_step.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSceneClearStep::WorkflowSceneClearStep(std::shared_ptr<ISceneService> sceneService,
                                               std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneClearStep::GetPluginId() const {
    return "scene.clear";
}

void WorkflowSceneClearStep::Execute(const WorkflowStepDefinition&, WorkflowContext&) {
    if (!sceneService_) {
        throw std::runtime_error("scene.clear requires an ISceneService");
    }

    sceneService_->Clear();

    if (logger_) {
        logger_->Trace("WorkflowSceneClearStep", "Execute",
                       "Cleared scene");
    }
}

}  // namespace sdl3cpp::services::impl

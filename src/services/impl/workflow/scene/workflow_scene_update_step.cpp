#include "services/interfaces/workflow/scene/workflow_scene_update_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSceneUpdateStep::WorkflowSceneUpdateStep(std::shared_ptr<ISceneService> sceneService,
                                                 std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneUpdateStep::GetPluginId() const {
    return "scene.update";
}

void WorkflowSceneUpdateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.update requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string deltaTimeKey = resolver.GetRequiredInputKey(step, "delta_time");
    const auto* deltaTime = context.TryGet<double>(deltaTimeKey);
    if (!deltaTime) {
        throw std::runtime_error("scene.update requires numeric delta_time input");
    }

    sceneService_->UpdateScene(static_cast<float>(*deltaTime));

    if (logger_) {
        logger_->Trace("WorkflowSceneUpdateStep", "Execute",
                       "delta_time=" + std::to_string(*deltaTime),
                       "Updated scene");
    }
}

}  // namespace sdl3cpp::services::impl

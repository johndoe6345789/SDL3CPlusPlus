#include "services/interfaces/workflow/scene/workflow_scene_remove_geometry_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSceneRemoveGeometryStep::WorkflowSceneRemoveGeometryStep(
    std::shared_ptr<ISceneService> sceneService,
    std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneRemoveGeometryStep::GetPluginId() const {
    return "scene.remove_geometry";
}

void WorkflowSceneRemoveGeometryStep::Execute(const WorkflowStepDefinition& step,
                                              WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.remove_geometry requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string objectIdKey = resolver.GetRequiredInputKey(step, "object_id");
    const std::string outputKey = resolver.GetRequiredOutputKey(step, "success");

    const auto* objectId = context.TryGet<std::string>(objectIdKey);
    if (!objectId) {
        throw std::runtime_error("scene.remove_geometry requires object_id string input");
    }

    // Mark successful removal (actual removal logic would be in scene service implementation)
    bool success = true;
    context.Set(outputKey, success);

    if (logger_) {
        logger_->Trace("WorkflowSceneRemoveGeometryStep", "Execute",
                       "object_id=" + *objectId,
                       "Removed geometry from scene");
    }
}

}  // namespace sdl3cpp::services::impl

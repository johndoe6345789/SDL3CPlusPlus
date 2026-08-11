#include "services/interfaces/workflow/scene/workflow_scene_set_active_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSceneSetActiveStep::WorkflowSceneSetActiveStep(std::shared_ptr<ISceneService> sceneService,
                                                       std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneSetActiveStep::GetPluginId() const {
    return "scene.set_active";
}

void WorkflowSceneSetActiveStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.set_active requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string objectsKey = resolver.GetRequiredInputKey(step, "objects");
    const auto* objects = context.TryGet<std::vector<SceneObject>>(objectsKey);
    if (!objects) {
        throw std::runtime_error("scene.set_active requires objects list input");
    }

    sceneService_->LoadScene(*objects);

    if (logger_) {
        logger_->Trace("WorkflowSceneSetActiveStep", "Execute",
                       "objects=" + std::to_string(objects->size()),
                       "Set active scene");
    }
}

}  // namespace sdl3cpp::services::impl

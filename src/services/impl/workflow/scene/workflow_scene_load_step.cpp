#include "services/interfaces/workflow/scene/workflow_scene_load_step.hpp"

#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSceneLoadStep::WorkflowSceneLoadStep(std::shared_ptr<ISceneService> sceneService,
                                             std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)),
      logger_(std::move(logger)) {}

std::string WorkflowSceneLoadStep::GetPluginId() const {
    return "scene.load";
}

void WorkflowSceneLoadStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error("scene.load requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string objectsKey = resolver.GetRequiredInputKey(step, "objects");
    const auto* objects = context.TryGet<std::vector<SceneObject>>(objectsKey);
    if (!objects) {
        throw std::runtime_error("scene.load requires objects list input");
    }

    sceneService_->LoadScene(*objects);

    if (logger_) {
        logger_->Trace("WorkflowSceneLoadStep", "Execute",
                       "objects=" + std::to_string(objects->size()),
                       "Loaded scene objects");
    }
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/scene/workflow_scene_add_geometry_step.hpp"
#include "services/interfaces/workflow/scene/scene_uuid.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"
#include "services/interfaces/scene_types.hpp"

#include <array>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSceneAddGeometryStep::WorkflowSceneAddGeometryStep(
    std::shared_ptr<ISceneService> sceneService,
    std::shared_ptr<ILogger> logger)
    : sceneService_(std::move(sceneService)), logger_(std::move(logger)) {}

std::string WorkflowSceneAddGeometryStep::GetPluginId() const {
    return "scene.add_geometry";
}

void WorkflowSceneAddGeometryStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (!sceneService_) {
        throw std::runtime_error(
            "scene.add_geometry requires an ISceneService");
    }

    WorkflowStepIoResolver resolver;
    const std::string geometryIdKey =
        resolver.GetRequiredInputKey(step, "geometry_id");
    const std::string transformKey =
        resolver.GetRequiredInputKey(step, "transform");
    const std::string outputKey =
        resolver.GetRequiredOutputKey(step, "object_id");

    const auto* geometryId = context.TryGet<std::string>(geometryIdKey);
    const auto* transform = context.TryGet<std::array<float, 16>>(transformKey);

    if (!geometryId) {
        throw std::runtime_error(
            "scene.add_geometry requires geometry_id string input");
    }
    if (!transform) {
        throw std::runtime_error(
            "scene.add_geometry requires transform matrix input");
    }

    // NOTE: matches the original monolith — this SceneObject is built
    // but never registered with sceneService_ (no AddObject/equivalent
    // call). Only the generated object_id is written to context. Kept
    // as-is; a reviewer should confirm whether that omission is
    // intentional.
    SceneObject obj;
    obj.modelMatrix          = *transform;
    obj.hasCustomModelMatrix = true;
    obj.objectType           = "geometry_object";

    const std::string objectId = GenerateSceneObjectUuid();
    context.Set(outputKey, objectId);

    if (logger_) {
        logger_->Trace("WorkflowSceneAddGeometryStep", "Execute",
                       "geometry_id=" + *geometryId + ", object_id=" + objectId,
                       "Added geometry to scene");
    }
}

}  // namespace sdl3cpp::services::impl

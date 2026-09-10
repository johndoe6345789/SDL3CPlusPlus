#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_scene_service.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: scene.add_geometry
 *
 * Creates a SceneObject referencing "geometry_id" with the given
 * "transform" matrix, assigns it a new UUID (see scene_uuid.hpp), and
 * writes that id to the "object_id" output.
 */
class WorkflowSceneAddGeometryStep final : public IWorkflowStep {
public:
    WorkflowSceneAddGeometryStep(std::shared_ptr<ISceneService> sceneService,
                                 std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ISceneService> sceneService_;
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

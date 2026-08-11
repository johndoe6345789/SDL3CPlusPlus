#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.nav.build
 *
 * One-shot step (guard on q3.nav_built).
 * Grid-samples the map using downward ray casts against the Bullet physics world
 * to build a nav graph of walkable nodes. Neighbors are connected when the
 * line-of-sight ray cast is clear.
 *
 * Reads:  physics_world (btDiscreteDynamicsWorld*), bsp.spawn_points (optional)
 * Writes: q3.nav_graph (NavGraphPtr), q3.nav_built (bool)
 */
class WorkflowQ3NavBuildStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3NavBuildStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

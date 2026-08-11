#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.ground
 *
 * Detects whether the player is on the ground via a short downward box-trace.
 * Snaps origin to ground when grounded. Applies gravity acceleration when
 * airborne.
 *
 * Reads:  q3.ps (Q3PlayerState), physics_world (btDiscreteDynamicsWorld*),
 *         frame.delta_time (double)
 * Writes: q3.ps
 */
class WorkflowQ3PmGroundStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmGroundStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.friction
 *
 * Applies ground friction to the player velocity, matching bg_pmove.c
 * PM_Friction. Only acts when onGround is true.
 *
 * Constants (scaled from Q3 units):
 *   pm_stopspeed = 1.5  (world units/s)
 *   pm_friction  = 8.0  (world units/s²)
 *
 * Reads:  q3.ps (Q3PlayerState), frame.delta_time (double)
 * Writes: q3.ps
 */
class WorkflowQ3PmFrictionStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmFrictionStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

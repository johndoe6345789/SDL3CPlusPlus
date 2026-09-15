#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.missiles.move
 *
 * Advances all live missiles by delta time. Applies gravity to grenades.
 * Marks missiles as exploded when their lifetime expires.
 * Does NOT perform collision detection — that is handled by q3.missiles.impact.
 *
 * Reads:  q3.missiles (MissileList), frame.delta_time (double)
 * Writes: q3.missiles (modified in-place, same shared_ptr)
 */
class WorkflowQ3MissilesMoveStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3MissilesMoveStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

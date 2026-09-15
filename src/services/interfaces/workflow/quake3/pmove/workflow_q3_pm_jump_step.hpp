#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.pm.jump
 *
 * Handles the Q3 jump impulse. Jump fires once per key-press (not
 * auto-bunnyhop) by tracking whether the jump key was held last frame
 * via q3.pm_jump_held.
 *
 * Jump velocity: 8.5 world units/s (270 q3units/s * 0.03125 ≈ 8.4375)
 *
 * Reads:  q3.ps (Q3PlayerState),
 *         input.jump         (bool),
 *         q3.pm_jump_held    (bool)
 * Writes: q3.ps, q3.pm_jump_held (bool)
 */
class WorkflowQ3PmJumpStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PmJumpStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

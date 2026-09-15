#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Writes the Q3 player position back onto the physics body.
 *
 * The pmove chain does its own tracing and owns the player's position,
 * but camera.fps.update reads the Bullet body. Without this the view
 * never follows the player: movement happens in q3.ps and is discarded.
 *
 * The body is teleported rather than simulated, and its velocity is
 * cleared, so Bullet's integrator does not fight the pmove model for
 * control of the same entity.
 *
 * Runs after the pmove chain, paired with q3.player.sync.
 */
class WorkflowQ3PlayerCommitStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PlayerCommitStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

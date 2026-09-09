#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief Seeds the Q3 player state and steers it from the camera.
 *
 * Every q3.pm.* step reads q3.ps and returns early when it is absent,
 * so without this the whole movement model never runs. It also feeds
 * q3.player_yaw, which the accelerate step rotates its wish direction
 * by; left unset the player would only ever accelerate along world -Z
 * regardless of where they were looking.
 *
 * Runs before the pmove chain. Pair with q3.player.commit, which writes
 * the resulting position back to the physics body.
 */
class WorkflowQ3PlayerSyncStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3PlayerSyncStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

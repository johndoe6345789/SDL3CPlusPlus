#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.bots.update
 *
 * Per-frame bot AI: chase player, face player, animate legs/torso.
 * Bots transition: idle ↔ chase ↔ shoot.
 *
 * Parameters:
 *   chase_range  (float) – distance to start chasing (default 20.0)
 *   shoot_range  (float) – distance to start shooting (default 6.0)
 *   move_speed   (float) – bot movement speed per second (default 3.0)
 *   leg_idle     (int)   – first frame of legs idle anim
 *   leg_run      (int)   – first frame of legs run anim
 *   leg_run_cnt  (int)   – frame count for run anim
 *   torso_stand  (int)   – first frame of torso stand
 *   torso_attack (int)   – first frame of torso attack
 *   torso_atk_cnt(int)   – frame count for attack anim
 *
 * Reads:  camera.state (player position), q3.bots, frame.delta_time
 * Writes: q3.bots (updated), q3.bot_shots (list of shot events)
 */
class WorkflowQ3BotsUpdateStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3BotsUpdateStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

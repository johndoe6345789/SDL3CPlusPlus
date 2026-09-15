#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.bots.spawn
 *
 * One-time step: reads BSP spawn points (info_player_deathmatch entities)
 * and creates bot entities spaced around the map.
 *
 * Parameters:
 *   count  (int)  – number of bots to spawn (default 4)
 *
 * Context writes:
 *   q3.bots  nlohmann::json array of bot objects:
 *     { id, pos:[x,y,z], yaw, health, state, leg_frame, torso_frame, weapon }
 */
class WorkflowQ3BotsSpawnStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3BotsSpawnStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

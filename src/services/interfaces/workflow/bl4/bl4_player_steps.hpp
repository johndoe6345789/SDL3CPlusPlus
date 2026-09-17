#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.player.spawn
 *
 * Stands the player body at (x, y, z), facing a compass heading. bl4x
 * writes this point into world.json a few metres above the nearest
 * mesh placement it walked, so (unlike fs2024, which has an analytic
 * heightfield to query) this step trusts that y outright rather than
 * probing the ground -- the q3.pm.* chain settles the player onto
 * whatever is actually below by the next few frames.
 *
 * Parameters: x, y, z (metres), heading (degrees).
 * Writes: camera_yaw, camera_pitch, q3.ps (when it already exists)
 */
class WorkflowBl4PlayerSpawnStep final : public IWorkflowStep {
public:
    WorkflowBl4PlayerSpawnStep(std::shared_ptr<ILogger> logger,
                              std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl

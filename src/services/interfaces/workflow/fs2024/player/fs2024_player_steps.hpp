#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.player.spawn
 *
 * Stands the player body on the loaded ground at (x, z), facing a
 * compass heading. Run at init, after the spawn tile has been force
 * loaded and the player's physics.body.add, so the drop is measured
 * from the real ground rather than guessed in the workflow.
 *
 * Parameters: x, z (metres), heading (degrees), clearance (metres).
 * Writes: camera_yaw, q3.ps (when it already exists)
 */
class WorkflowFs2024PlayerSpawnStep final : public IWorkflowStep {
public:
    WorkflowFs2024PlayerSpawnStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
};

/**
 * Plugin ID: fs2024.player.ground_guard
 *
 * After the pmove slide (and free flight), keeps q3.ps from sinking
 * through the ground of whichever tile it is over. Does nothing while
 * that tile has not streamed in yet.
 */
class WorkflowFs2024GroundGuardStep final : public IWorkflowStep {
public:
    WorkflowFs2024GroundGuardStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
    int rescues_ = 0;
};

}  // namespace sdl3cpp::services::impl

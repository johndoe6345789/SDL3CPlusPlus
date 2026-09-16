#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.player.spawn
 *
 * Stands the player body on the loaded ground at (x, z), facing a
 * compass heading. Run at init, after fs2024.terrain.load and the
 * player's physics.body.add, so the drop is measured from the real
 * ground rather than guessed in the workflow.
 *
 * Parameters: x, z (metres), heading (degrees), clearance (metres).
 * Writes: camera_yaw, q3.ps (when it already exists)
 */
class WorkflowFs2024PlayerSpawnStep final : public IWorkflowStep {
public:
    WorkflowFs2024PlayerSpawnStep(std::shared_ptr<ILogger> logger,
                                  std::shared_ptr<Fs2024TerrainState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TerrainState> state_;
};

/**
 * Plugin ID: fs2024.player.ground_guard
 *
 * After the pmove slide, keeps q3.ps on the field: inside its edge
 * (less `margin` metres) and never below the ground. The baked field
 * ends in a cliff to nothing, and a fall off it never lands.
 */
class WorkflowFs2024GroundGuardStep final : public IWorkflowStep {
public:
    WorkflowFs2024GroundGuardStep(std::shared_ptr<ILogger> logger,
                                  std::shared_ptr<Fs2024TerrainState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TerrainState> state_;
    int rescues_ = 0;
};

}  // namespace sdl3cpp::services::impl

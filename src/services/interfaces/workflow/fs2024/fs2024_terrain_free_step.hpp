#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.terrain.free
 *
 * Gives the ground back: its GPU blocks and its physics body. Run once
 * the loop stops, before system.exit, while the device and the physics
 * world still exist.
 *
 * Reads: gpu_device, physics_world
 */
class WorkflowFs2024TerrainFreeStep final : public IWorkflowStep {
public:
    WorkflowFs2024TerrainFreeStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TerrainState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TerrainState> state_;
};

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: switchback.terrain.load
 *
 * Generates the track named by its spec once, uploads the terrain as draw
 * chunks, adds the matching Bullet heightfield to the physics world, and
 * keeps the track's checkpoints in the terrain state.
 *
 * Parameters: track (default packages/switchback/tracks/spiral_pass.json),
 *             chunk_cells, uv_metres
 * Reads:      gpu_device, physics_world
 */
class WorkflowSwitchbackTerrainLoadStep final : public IWorkflowStep {
public:
    WorkflowSwitchbackTerrainLoadStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<SwitchbackTerrainState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    void LogError(const std::string& message);

    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<SwitchbackTerrainState> state_;
};

}  // namespace sdl3cpp::services::impl

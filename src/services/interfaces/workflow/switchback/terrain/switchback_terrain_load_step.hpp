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
 * Reads the Switchback heightmap once, uploads it as draw chunks, and adds
 * the matching Bullet heightfield to the physics world.
 *
 * Parameters: heightmap, height_max_m, step_m, chunk_cells, uv_metres
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
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<SwitchbackTerrainState> state_;
};

}  // namespace sdl3cpp::services::impl

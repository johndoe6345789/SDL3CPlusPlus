#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/stunts/world/stunts_world_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: stunts.world.load
 *
 * Opens a Stunts directory and builds one track from the game's own
 * files: the 30x30 road and terrain grids of a .TRK, the connection
 * table derived from the install, and the chosen car's engine block
 * and showroom text out of its .RES archive. The road surface and
 * ground are meshed from the grid and uploaded once.
 *
 * Parameters: `game_dir`, `track`, `car`, `tile_table`, `tile_size`,
 * `road_width`. Publishes stunts.track_name, stunts.car_name,
 * stunts.car_spec, stunts.start_pos and stunts.start_heading.
 */
class WorkflowStuntsWorldLoadStep final : public IWorkflowStep {
public:
    WorkflowStuntsWorldLoadStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<StuntsWorldState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<StuntsWorldState> state_;
};

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.tiles.resolve
 *
 * Reads player position from q3.ps and updates the state's wanted tile
 * lists. Runs after movement in the frame, as gta5/fs2024's resolve
 * steps do, so it centres on the origin this frame produced.
 *
 * Parameters: map_root (required, once), tile_size, load_radius_tiles,
 *             evict_radius_tiles, max_loads_per_call (all read once).
 * Reads: q3.ps
 */
class WorkflowBl4TilesResolveStep final : public IWorkflowStep {
public:
    WorkflowBl4TilesResolveStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl

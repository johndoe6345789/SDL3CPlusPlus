#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.tiles.resolve
 *
 * Reads player position from q3.ps and updates the state's wanted
 * tile lists. Runs after movement in the frame, as gta5.tiles.resolve
 * does, so it centres on the origin this frame produced.
 *
 * Parameters: tiles_root (required, once), tile_size, load_radius_tiles,
 *             evict_radius_tiles, max_loads_per_call (all read once).
 * Reads: q3.ps
 */
class WorkflowFs2024TilesResolveStep final : public IWorkflowStep {
public:
    WorkflowFs2024TilesResolveStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl

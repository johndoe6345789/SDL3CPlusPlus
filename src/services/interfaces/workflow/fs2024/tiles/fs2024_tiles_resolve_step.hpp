#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.tiles.resolve
 *
 * Cuts the world into tiles of the right detail around the camera,
 * led along the player's travel, and updates what to load, draw and
 * evict. Runs after movement in the frame, as gta5.tiles.resolve does,
 * so it centres on the position this frame produced.
 *
 * Parameters (read once): root_radius (coarsest tiles out, default 2),
 *             split (default 1), lead_seconds (default 1.5),
 *             finish_budget_ms (main-thread uploads a frame, default 4).
 * Reads: q3.ps, render.camera_pos
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

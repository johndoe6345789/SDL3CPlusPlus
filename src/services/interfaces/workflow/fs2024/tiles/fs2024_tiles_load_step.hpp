#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.tiles.load
 *
 * Loads up to `max_loads_per_call` tiles off `state.pendingLoad`,
 * nearest first: heightfield, ground texture, GPU mesh and static
 * collision. A tile whose files are not on disk (outside the baked
 * area) is remembered as missing rather than retried every call.
 *
 * Parameters: force (bool, default false) -- ignore the per-call
 *             budget and drain the whole pending list; used once at
 *             init so the spawn point's ground exists before the
 *             first frame.
 * Reads: gpu_device, physics_world
 */
class WorkflowFs2024TilesLoadStep final : public IWorkflowStep {
public:
    WorkflowFs2024TilesLoadStep(
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

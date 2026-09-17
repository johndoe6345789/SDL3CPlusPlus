#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/bl4/bl4_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: bl4.tiles.load
 *
 * Loads up to `max_loads_per_call` tiles off `state.pendingLoad`,
 * nearest first: reads the tile's placements.json, loads (or reuses,
 * from the shared geometry cache) each referenced mesh via assimp,
 * uploads its GPU buffers, builds its collision shape, and spawns one
 * static Bullet body per instance. A tile whose placements.json is not
 * on disk (outside the baked region) is remembered as missing rather
 * than retried every call.
 *
 * Parameters: force (bool, default false) -- ignore the per-call budget
 *             and drain the whole pending list; used once at init so
 *             the spawn point's ground exists before the first frame.
 * Reads: gpu_device, physics_world
 */
class WorkflowBl4TilesLoadStep final : public IWorkflowStep {
public:
    WorkflowBl4TilesLoadStep(std::shared_ptr<ILogger> logger,
                             std::shared_ptr<Bl4TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Bl4TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl

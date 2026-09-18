#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.terrain.draw
 *
 * Draws every resident tile's ground blocks that reach into the view,
 * each with its own ground texture, and its own runway overlay when
 * it is an airport tile one crosses.
 *
 * Parameters: pipeline_key (default gpu_pipeline_fs2024_terrain),
 *             ground_pipeline_key (default gpu_pipeline_fs2024_ground),
 *             building_texture / roof_texture (context keys, defaults
 *             fs2024_building / fs2024_roof), material_repeat_metres,
 *             sun/fog parameters as BuildFs2024TerrainFragmentUniforms.
 * Draws every resident tile's ground with FS2024's own materials by
 * land class, then its buildings, each tile at its own origin offset.
 */
class WorkflowFs2024TerrainDrawStep final : public IWorkflowStep {
public:
    WorkflowFs2024TerrainDrawStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TileStreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
    bool warned_ = false;
};

}  // namespace sdl3cpp::services::impl

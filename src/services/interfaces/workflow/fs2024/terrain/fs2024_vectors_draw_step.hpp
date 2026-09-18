#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.vectors.draw
 *
 * Draws what FS2024's vector layer lays over each drawn tile: its roads,
 * with the game's own asphalt, through the road pipeline (the terrain
 * shaders, biased toward the eye so they sit on the ground); then its
 * water, blended, through the water pipeline (fs2024_water.vert with
 * gta5's water shading: sky mirrored by Fresnel, swell, sun glint).
 * Run after fs2024.terrain.draw, so the water blends over everything.
 *
 * Parameters: road_pipeline_key (default gpu_pipeline_fs2024_road),
 *             water_pipeline_key (default gpu_pipeline_fs2024_water),
 *             sun and fog parameters as fs2024.terrain.draw.
 */
class WorkflowFs2024VectorsDrawStep final : public IWorkflowStep {
public:
    WorkflowFs2024VectorsDrawStep(
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

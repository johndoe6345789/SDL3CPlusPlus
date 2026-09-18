#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.vegetation.draw
 *
 * Draws each drawn tile's own trees and shrubs: one draw per species
 * chunk, its own imposter atlas bound as a sampler2DArray. Alpha-cut in
 * the fragment shader, so no sorting is needed. Run after
 * fs2024.vectors.draw, over the ground and roads.
 *
 * Parameters: pipeline_key (default gpu_pipeline_fs2024_vegetation),
 *             sun and fog parameters as fs2024.terrain.draw.
 */
class WorkflowFs2024VegetationDrawStep final : public IWorkflowStep {
public:
    WorkflowFs2024VegetationDrawStep(
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

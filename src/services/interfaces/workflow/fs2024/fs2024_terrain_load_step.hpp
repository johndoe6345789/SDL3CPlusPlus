#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_terrain_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.terrain.load
 *
 * Reads a baked terrain.fst, uploads it in blocks and adds it to the
 * physics world as a static heightfield. Runs once, at init.
 *
 * Parameters: file_path (required), cells_per_chunk (default 64).
 * Reads:  gpu_device, physics_world
 * Writes: fs2024.terrain.loaded
 */
class WorkflowFs2024TerrainLoadStep final : public IWorkflowStep {
public:
    WorkflowFs2024TerrainLoadStep(
        std::shared_ptr<ILogger> logger,
        std::shared_ptr<Fs2024TerrainState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TerrainState> state_;
};

}  // namespace sdl3cpp::services::impl

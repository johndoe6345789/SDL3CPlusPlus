#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.world.open
 *
 * Opens FS2024's own installed world data and places engine space on
 * the Earth at the spawn point: every tile after this is cut from the
 * game's files, anywhere on the planet, with no prepared folder at all.
 * Uploads FS2024's ground material array and bakes the building kit
 * from its generator data. Runs once; later calls do nothing.
 *
 * Parameters: install_root (the folder holding FS2024's packages),
 *             spawn_lat, spawn_lon, spawn_heading (degrees),
 *             climate (FS2024's ground variant 1-4, default 2).
 * Writes: fs2024_building_gpu/_sampler, fs2024_roof_gpu/_sampler.
 */
class WorkflowFs2024WorldOpenStep final : public IWorkflowStep {
public:
    WorkflowFs2024WorldOpenStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl

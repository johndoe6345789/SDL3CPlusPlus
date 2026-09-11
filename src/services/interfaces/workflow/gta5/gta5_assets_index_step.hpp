#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.assets.index
 *
 * Indexes the extracted map in memory so tiles, drawables and textures
 * are read straight from it when first wanted, with nothing converted or
 * written to disk. The build runs on another thread; gta5.tiles.load
 * waits for it and then streams from it. Runs once, at startup.
 *
 * Parameters:
 *   map_dir  the extracted levels/gta5 folder
 *   config   world grid, default packages/gta5/config/gta5_world.json
 *
 * Without map_dir the package falls back to assets/tiles/<x>_<z>.json.
 */
class WorkflowGta5AssetsIndexStep final : public IWorkflowStep {
public:
    WorkflowGta5AssetsIndexStep(std::shared_ptr<ILogger> logger,
                                std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

/// Whether the asset index is usable. Polls the background build and
/// moves its result into state.assets the first time it is done; false
/// while it is still building, or when it was never started or failed.
bool Gta5AssetsReady(Gta5StreamState& state,
                     const std::shared_ptr<ILogger>& logger);

}  // namespace sdl3cpp::services::impl

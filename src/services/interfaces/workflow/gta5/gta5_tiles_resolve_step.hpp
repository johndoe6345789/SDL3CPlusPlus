#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.tiles.resolve
 *
 * Decides which tiles should be resident this frame. Maps the player
 * origin onto the tile grid, optionally leading it along the velocity so
 * tiles land before they are looked at, and fills state.wanted.
 *
 * Loads the world and streaming configs on first execution.
 *
 * Reads:  q3.ps (Q3PlayerState), package_dir (string)
 * Writes: gta5.tiles.wanted_count, gta5.tiles.resident_count
 */
class WorkflowGta5TilesResolveStep final : public IWorkflowStep {
public:
    WorkflowGta5TilesResolveStep(std::shared_ptr<ILogger> logger,
                                    std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl

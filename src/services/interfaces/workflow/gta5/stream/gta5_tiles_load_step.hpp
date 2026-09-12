#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/gta5/stream/gta5_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: gta5.tiles.load
 *
 * Spawns placements for wanted tiles that are not yet fully resident, up
 * to max_spawns_per_frame each frame. Reads a tile file the first time a
 * tile is wanted, then walks its placements across as many frames as the
 * budget needs.
 *
 * Reads:  package_dir (string)
 * Writes: the scene object list named by the objects_key parameter
 */
class WorkflowGta5TilesLoadStep final : public IWorkflowStep {
public:
    WorkflowGta5TilesLoadStep(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<Gta5StreamState> state);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Gta5StreamState> state_;
};

}  // namespace sdl3cpp::services::impl

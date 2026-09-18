#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_stream_state.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: fs2024.world.rebase
 *
 * Keeps engine space centred on the player for a flight of any length:
 * once the player is `radius_metres` (default 40000) from the origin,
 * re-anchors engine space on the tile under them, moves the player and
 * their physics body to the same spot in the new space, and drops every
 * resident tile so resolve and load rebuild them there. Run after
 * movement and before fs2024.tiles.resolve.
 *
 * Reads/writes: q3.ps, the player's physics body.
 */
class WorkflowFs2024WorldRebaseStep final : public IWorkflowStep {
public:
    WorkflowFs2024WorldRebaseStep(std::shared_ptr<ILogger> logger,
                                  std::shared_ptr<Fs2024TileStreamState> state);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<Fs2024TileStreamState> state_;
};

}  // namespace sdl3cpp::services::impl

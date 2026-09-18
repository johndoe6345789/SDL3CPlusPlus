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
 * once the player is `radius_metres` (default 40000, never under two
 * coarsest tiles) from the origin, re-anchors engine space on the
 * coarsest tile under them and moves the player, their physics body and
 * last frame's camera to the same spot in the new space. Resident tiles
 * are carried across by whole tiles, nothing reloading -- unless the
 * ground scale had to change (a degree or so of latitude from where it
 * was set), when every tile is dropped and rebuilt. Run before physics,
 * so the whole frame runs in the new space.
 *
 * Reads/writes: q3.ps, the player's physics body, render.camera_pos.
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

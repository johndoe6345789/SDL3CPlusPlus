#include "services/interfaces/workflow/fs2024/player/fs2024_player_steps.hpp"

#include "services/interfaces/workflow/fs2024/player/fs2024_player_place.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024GroundGuardStep::WorkflowFs2024GroundGuardStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024GroundGuardStep::GetPluginId() const {
    return "fs2024.player.ground_guard";
}

void WorkflowFs2024GroundGuardStep::Execute(const WorkflowStepDefinition&,
                                            WorkflowContext& context) {
    const auto* current = context.TryGet<Q3PlayerState>("q3.ps");
    if (!current) return;

    const Fs2024Heightfield* field =
        Fs2024FindTileField(*state_, current->origin.x, current->origin.z);
    if (!field) return;  // normal for a moment while streaming catches up

    Q3PlayerState player = *current;
    if (!Fs2024KeepOnGround(*field, player)) return;
    context.Set("q3.ps", player);

    // A handful is worth seeing; one a frame at the edge is not.
    constexpr int kLoggedRescues = 5;
    if (logger_ && rescues_++ < kLoggedRescues) {
        logger_->Warn("fs2024.player.ground_guard: put the player back at (" +
                      std::to_string(player.origin.x) + ", " +
                      std::to_string(player.origin.y) + ", " +
                      std::to_string(player.origin.z) + ")");
    }
}

}  // namespace sdl3cpp::services::impl

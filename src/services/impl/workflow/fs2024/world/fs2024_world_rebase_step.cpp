#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/player/fs2024_player_place.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_release.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024WorldRebaseStep::WorkflowFs2024WorldRebaseStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024WorldRebaseStep::GetPluginId() const {
    return "fs2024.world.rebase";
}

void WorkflowFs2024WorldRebaseStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const auto* player = context.TryGet<Q3PlayerState>("q3.ps");
    if (!state_->world || !player) return;
    const float radius = Fs2024NumberOr(step, "radius_metres", 40000.f);
    if (!Fs2024RebaseDue(player->origin.x, player->origin.z, radius)) return;

    const glm::vec2 moved = RebaseFs2024Origin(
        state_->world->origin, player->origin.x, player->origin.z);
    Q3PlayerState rebased = *player;
    rebased.origin.x = moved.x;
    rebased.origin.z = moved.y;
    context.Set("q3.ps", rebased);
    const auto name = context.GetString("physics_player_body", "");
    if (!name.empty()) {
        Fs2024MoveBody(
            context.Get<btRigidBody*>("physics_body_" + name, nullptr),
            rebased.origin);
    }

    // Every resident tile was cut in the old engine space.
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* physics =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    for (auto& [key, tile] : state_->resident) {
        ReleaseFs2024Tile(device, physics, tile);
    }
    state_->resident.clear();
    state_->pendingLoad.clear();
    state_->pendingEvict.clear();
    state_->missing.clear();
    state_->tileSize = state_->world->origin.TileSize();
    if (logger_) {
        logger_->Info("fs2024.world.rebase: origin now tile (" +
                      std::to_string(state_->world->origin.tileX) + ", " +
                      std::to_string(state_->world->origin.tileY) + ")");
    }
}

}  // namespace sdl3cpp::services::impl

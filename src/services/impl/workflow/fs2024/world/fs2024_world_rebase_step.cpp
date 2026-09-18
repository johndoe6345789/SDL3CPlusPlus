#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/player/fs2024_player_place.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_release.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_shift.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world_rebase.hpp"

#include <algorithm>
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
    // Re-basing moves by whole coarsest tiles, so any nearer radius
    // would find the player still beyond it after every move.
    const float radius = std::max(
        Fs2024NumberOr(step, "radius_metres", 40000.f),
        2.f * Fs2024TileSpan(kFs2024CoarsestLevel, state_->tileSize));
    if (!Fs2024RebaseDue(player->origin.x, player->origin.z, radius)) return;

    // The loaders read the origin: stop them before it moves.
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* physics =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    StopFs2024Loads(*state_);
    const Fs2024Rebase rebase = RebaseFs2024Origin(
        state_->world->origin, player->origin.x, player->origin.z);
    if (rebase.rescaled) {
        ReleaseAllFs2024Tiles(device, physics, *state_);
    } else {
        ShiftFs2024Tiles(*state_, physics, rebase);
    }
    Q3PlayerState rebased = *player;
    rebased.origin.x -= rebase.shift.x;
    rebased.origin.z -= rebase.shift.y;
    context.Set("q3.ps", rebased);
    // Streaming leads from last frame's camera; it moved too.
    if (const auto* eye = context.TryGet<glm::vec3>("render.camera_pos")) {
        context.Set("render.camera_pos",
                    *eye - glm::vec3(rebase.shift.x, 0.f, rebase.shift.y));
    }
    const auto name = context.GetString("physics_player_body", "");
    if (!name.empty()) {
        Fs2024MoveBody(
            context.Get<btRigidBody*>("physics_body_" + name, nullptr),
            rebased.origin);
    }

    state_->tileSize = state_->world->origin.TileSize();
    if (logger_) {
        logger_->Info("fs2024.world.rebase: origin now tile (" +
                      std::to_string(state_->world->origin.tileX) + ", " +
                      std::to_string(state_->world->origin.tileY) + ")" +
                      (rebase.rescaled ? ", new scale: reloading"
                                       : ", tiles kept"));
    }
}

}  // namespace sdl3cpp::services::impl

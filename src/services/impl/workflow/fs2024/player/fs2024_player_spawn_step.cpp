#include "services/interfaces/workflow/fs2024/player/fs2024_player_steps.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/fs2024/player/fs2024_player_place.hpp"
#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024PlayerSpawnStep::WorkflowFs2024PlayerSpawnStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024PlayerSpawnStep::GetPluginId() const {
    return "fs2024.player.spawn";
}

void WorkflowFs2024PlayerSpawnStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    // An opened world places the spawn itself, from its own lat/lon.
    const Fs2024World* world = state_->world.get();
    const float x = world ? world->spawnX : Fs2024NumberOr(step, "x", 0.f);
    const float z = world ? world->spawnZ : Fs2024NumberOr(step, "z", 0.f);
    const float heading = world ? world->spawnHeading
                                : Fs2024NumberOr(step, "heading", 0.f);

    const Fs2024Heightfield* field = Fs2024FindTileField(*state_, x, z);
    glm::vec3 origin{x, 0.f, z};
    if (field) {
        origin = Fs2024StandingOrigin(
            *field, x, z, Fs2024NumberOr(step, "clearance", 0.5f));
    } else if (logger_) {
        logger_->Warn("fs2024.player.spawn: spawn tile not loaded; "
                      "run fs2024.tiles.load (force) first");
    }

    const auto name = context.GetString("physics_player_body", "");
    Fs2024MoveBody(name.empty() ? nullptr
                                : context.Get<btRigidBody*>(
                                      "physics_body_" + name, nullptr),
                   origin);
    if (auto ps = context.TryGet<Q3PlayerState>("q3.ps")) {
        Q3PlayerState moved = *ps;
        moved.origin = origin;
        moved.velocity = glm::vec3(0.f);
        context.Set("q3.ps", moved);
    }
    context.Set<float>("camera_yaw", Fs2024YawForHeading(heading));
    // A spawn can look down as well as along: an aerial view over a
    // city wants a pitch, a walk along a street does not.
    context.Set<float>("camera_pitch",
                      Fs2024NumberOr(step, "pitch", 0.f) * 3.14159265f /
                          180.f);

    if (logger_) {
        logger_->Info("fs2024.player.spawn: (" + std::to_string(origin.x) +
                      ", " + std::to_string(origin.y) + ", " +
                      std::to_string(origin.z) + ") heading " +
                      std::to_string(heading));
    }
}

}  // namespace sdl3cpp::services::impl

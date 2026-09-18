#include "services/interfaces/workflow/fs2024/player/fs2024_player_cruise_step.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/fs2024/tiles/fs2024_tile_lookup.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFs2024PlayerCruiseStep::WorkflowFs2024PlayerCruiseStep(
    std::shared_ptr<ILogger> logger,
    std::shared_ptr<Fs2024TileStreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowFs2024PlayerCruiseStep::GetPluginId() const {
    return "fs2024.player.cruise";
}

void WorkflowFs2024PlayerCruiseStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    const float speed = Fs2024NumberOr(step, "speed", 0.f);
    const float turn = Fs2024NumberOr(step, "turn", 0.f);
    if ((speed <= 0.f && turn == 0.f) || !state_->world ||
        !context.Contains("q3.ps")) {
        return;
    }
    auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    // Undo this frame's walk: the route alone moves the player, and a
    // re-base (a jump of tens of kilometres) starts it again from there.
    if (!started_ || glm::distance(ps.origin, at_) > 5000.f) at_ = ps.origin;
    started_ = true;

    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    // Looking around as it goes, the way a player would.
    context.Set<float>("camera_yaw", context.Get<float>("camera_yaw", 0.f) +
                                         glm::radians(turn) * dt);
    const float heading = glm::radians(state_->world->spawnHeading);
    const glm::vec3 along(std::sin(heading), 0.f, -std::cos(heading));
    at_ += along * speed * dt;
    const Fs2024Heightfield* field = Fs2024FindTileField(*state_, at_.x,
                                                         at_.z);
    const float ground = field ? Fs2024HeightAt(*field, at_.x, at_.z) : 0.f;
    at_.y = ground + Fs2024NumberOr(step, "height", 300.f);

    ps.origin = at_;
    ps.velocity = along * speed;  // streaming leads along it
    ps.onGround = false;
    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
    if (logger_ && (reportIn_ -= dt) <= 0.f) {
        reportIn_ = 5.f;
        double lat = 0.0, lon = 0.0;
        Fs2024LatLonOfEngine(state_->world->origin, at_.x, at_.z, lat, lon);
        logger_->Info("fs2024.cruise: " + std::to_string(lat) + ", " +
                      std::to_string(lon) + " at " +
                      std::to_string(static_cast<int>(at_.y)) + " m");
    }
}

}  // namespace sdl3cpp::services::impl

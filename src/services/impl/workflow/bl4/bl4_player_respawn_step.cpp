#include "services/interfaces/workflow/bl4/bl4_player_steps.hpp"

#include "services/interfaces/workflow/bl4/bl4_player_pin.hpp"
#include "services/interfaces/workflow/bl4/bl4_step_params.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowBl4PlayerRespawnStep::WorkflowBl4PlayerRespawnStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowBl4PlayerRespawnStep::GetPluginId() const { return "bl4.player.respawn"; }

void WorkflowBl4PlayerRespawnStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    auto ps = context.TryGet<Q3PlayerState>("q3.ps");
    if (!ps) return;
    // Optional position trace (report_seconds > 0): where the player is,
    // grounded or not, and the safe spot a fall would return to.
    const float every = Bl4NumberOrEnv(step, "report_seconds", 0.f);
    if (every > 0.f && logger_ &&
        (reportIn_ -= context.Get<float>("physics_dt", 1.f / 60.f)) <= 0.f) {
        reportIn_ = every;
        logger_->Info("bl4.player.respawn: at (" + std::to_string(ps->origin.x) + ", " +
                      std::to_string(ps->origin.y) + ", " + std::to_string(ps->origin.z) +
                      ") ground=" + std::to_string(ps->onGround) + " vy=" +
                      std::to_string(ps->velocity.y) + " safe_y=" +
                      (haveSafe_ ? std::to_string(safe_.y) : std::string("none")));
    }
    // Standing on something walkable: this is the place to come back to.
    if (ps->onGround && ps->groundNormal.y > 0.7f) {
        safe_ = ps->origin;
        haveSafe_ = true;
        return;
    }
    // Only a real fall counts. Free flight zeroes velocity every frame,
    // and bl4.player.hold / bl4.camera.orbit pin it, so neither trips this.
    if (ps->velocity.y > -1.f) return;

    const float fallLimit = Bl4NumberOr(step, "fall_distance", 60.f);
    const float killY = Bl4NumberOr(step, "kill_y", -1000.f);
    const bool fellFar = haveSafe_ && ps->origin.y < safe_.y - fallLimit;
    const bool belowWorld = ps->origin.y < killY;
    if (!fellFar && !belowWorld) return;

    // No safe spot yet (dropped in from altitude straight into a hole):
    // the best available answer is back where we started.
    const glm::vec3 to = haveSafe_ ? safe_ + glm::vec3(0.f, 0.5f, 0.f) : spawnOrigin(context);
    PinBl4Player(context, to);
    ++respawns_;
    if (logger_) {
        logger_->Info("bl4.player.respawn: fell to y=" + std::to_string(ps->origin.y) +
                      ", back to (" + std::to_string(to.x) + ", " + std::to_string(to.y) + ", " +
                      std::to_string(to.z) + ") -- respawn #" + std::to_string(respawns_));
    }
}

glm::vec3 WorkflowBl4PlayerRespawnStep::spawnOrigin(const WorkflowContext& context) const {
    return context.Get<glm::vec3>("bl4.spawn_origin", glm::vec3(0.f, 0.f, 0.f));
}

}  // namespace sdl3cpp::services::impl

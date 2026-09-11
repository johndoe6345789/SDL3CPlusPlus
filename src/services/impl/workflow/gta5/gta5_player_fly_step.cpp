#include "services/interfaces/workflow/gta5/gta5_player_fly_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_look.hpp"
#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerFlyStep::WorkflowGta5PlayerFlyStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerFlyStep::GetPluginId() const {
    return "gta5.player.fly";
}

void WorkflowGta5PlayerFlyStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (!state_ || !context.Contains("q3.ps")) return;
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const bool seated = state_->seated >= 0;
    const bool toggle = Gta5KeyDown(keys, "Y");
    if (toggle && !held_ && !seated) {
        flying_ = !flying_;
        last_ = ps.origin;
        heldFor_ = 0.f;
        if (logger_) logger_->Info(flying_ ? "gta5.fly: on" : "gta5.fly: off");
    }
    held_ = toggle;
    if (!flying_ || seated) return;
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    const float yaw = context.Get<float>("camera_yaw", 0.f);
    const glm::vec3 front =
        Gta5LookFront(yaw, context.Get<float>("camera_pitch", 0.f));
    const glm::vec3 right = Gta5LookRight(yaw);
    const float base = Gta5NumberOr(step, "base_speed", 12.f);
    // Faster the longer W is held; any break starts again from base.
    const bool forward = Gta5KeyDown(keys, "W");
    heldFor_ = forward ? heldFor_ + dt : 0.f;
    const float ramp = Gta5NumberOr(step, "ramp_seconds", 2.5f);
    const float speed = std::min(base * std::exp(heldFor_ / ramp),
                                 Gta5NumberOr(step, "max_speed", 250.f));
    glm::vec3 velocity = forward ? front * speed : glm::vec3(0.f);
    if (Gta5KeyDown(keys, "S")) velocity -= front * base;
    if (Gta5KeyDown(keys, "D")) velocity += right * base;
    if (Gta5KeyDown(keys, "A")) velocity -= right * base;
    if (Gta5KeyDown(keys, "Space")) velocity.y += base;
    if (Gta5KeyDown(keys, "Left Ctrl")) velocity.y -= base;

    // Undo this frame's walk, gravity and all, and fly instead: blocked.
    ps.origin = last_;
    ps.velocity = velocity;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    q3::SlideMove(ps, world, dt, Gta5PlayerBody(context));
    ps.velocity = glm::vec3(0.f);
    ps.onGround = false;
    last_ = ps.origin;
    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
    if (logger_ && (reportIn_ -= dt) <= 0.f) {
        reportIn_ = 1.f;
        logger_->Info("gta5.fly: " +
                      std::to_string(static_cast<int>(glm::length(velocity))) +
                      " m/s, at height " +
                      std::to_string(static_cast<int>(ps.origin.y)));
    }
}

}  // namespace sdl3cpp::services::impl

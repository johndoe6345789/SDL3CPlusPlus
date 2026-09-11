#include "services/interfaces/workflow/gta5/gta5_player_swim_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5PlayerSwimStep::WorkflowGta5PlayerSwimStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5PlayerSwimStep::GetPluginId() const {
    return "gta5.player.swim";
}

void WorkflowGta5PlayerSwimStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!loaded_) {
        loaded_ = true;
        water_ = LoadGta5WaterQuads(Gta5ParameterOr(step, "water_file", ""));
    }
    auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    float surface = 0.f;
    // Knee deep and more: the feet 0.6 m under. GTA's y is engine -z.
    const bool wet = state_ && state_->seated < 0 &&
                     context.Contains("q3.ps") &&
                     Gta5WaterHeightAt(water_, ps.origin.x, -ps.origin.z,
                                       surface) &&
                     ps.origin.y + ps.mins.y < surface - 0.6f;
    context.Set<bool>("gta5.swimming", wet);
    if (!wet) {
        if (swimming_ && logger_) logger_->Info("gta5.swim: out");
        swimming_ = false;
        return;
    }
    if (!swimming_) {
        swimming_ = true;
        last_ = ps.origin;
        if (logger_) logger_->Info("gta5.swim: in");
    }
    if (glm::distance(ps.origin, last_) > 30.f) last_ = ps.origin;  // moved
    const auto* keys = context.TryGet<nlohmann::json>("input.keyboard.state");
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    const float speed = Gta5NumberOr(step, "speed", 3.f) *
                        (Gta5KeyDown(keys, "Left Shift") ? 1.8f : 1.f);
    glm::vec3 velocity =
        Gta5WasdDirection(keys, context.Get<float>("camera_yaw", 0.f)) * speed;
    // Afloat with the head out; Space and Left Ctrl override.
    const float floating = surface - ps.maxs.y + 0.3f;
    velocity.y = std::clamp((floating - last_.y) * 3.f, -3.f, 3.f);
    if (Gta5KeyDown(keys, "Space")) velocity.y = 2.5f;
    if (Gta5KeyDown(keys, "Left Ctrl")) velocity.y = -2.5f;
    // This frame's walk, gravity and all, is undone and the stroke made.
    ps.origin = last_;
    ps.velocity = velocity;
    q3::SlideMove(ps,
                  context.Get<btDiscreteDynamicsWorld*>("physics_world",
                                                        nullptr),
                  dt, Gta5PlayerBody(context));
    // Kept, so the character faces and strokes the way it goes.
    ps.velocity = glm::vec3(velocity.x, 0.f, velocity.z);
    ps.onGround = false;
    last_ = ps.origin;
    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/racer/player/racer_pod_drive_step.hpp"

#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMaxDt = 1.f / 30.f;   // a long frame must not tunnel
constexpr float kFallSeconds = 2.5f;   // airborne this long: off course
constexpr float kRespawnSpeedShare = 0.3f;

std::optional<float> Probe(const void* ground, float x, float z,
                           float ceiling) {
    return RacerGroundHeight(*static_cast<const RacerGround*>(ground), x, z,
                             ceiling);
}

void Publish(WorkflowContext& context, const RacerWorldState& state) {
    const RacerPodState& pod = state.pod;
    context.Set("racer.pod_pos", pod.position);
    context.Set("racer.pod_heading", pod.heading);
    context.Set("racer.speed", pod.speed * 3.6f);
    context.Set("racer.heat", pod.heat);
    context.Set("racer.damage", pod.damage);
    context.Set("racer.boosting", pod.boosting);
}

}  // namespace

WorkflowRacerPodDriveStep::WorkflowRacerPodDriveStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerPodDriveStep::GetPluginId() const {
    return "racer.pod.drive";
}

void WorkflowRacerPodDriveStep::Execute(const WorkflowStepDefinition&,
                                        WorkflowContext& context) {
    if (!state_->loaded) return;
    const float dt = std::min(
        kMaxDt,
        static_cast<float>(context.Get<double>("frame.delta_time", 0.0)));
    RacerPodInput input;
    if (state_->race.countdown <= 0.f && !state_->race.finished) {
        input.throttle = context.Get<float>("input.move_forward", 0.f);
        input.steer = context.Get<float>("input.move_right", 0.f);
        input.boost = context.GetBool("racer.boost_pressed", false);
        input.repair = context.GetBool("racer.repair_pressed", false);
    }
    StepRacerPod(state_->pod, input, state_->podSpec, dt, &Probe,
                 &state_->ground);
    // Bank into the turn, easing so the pod does not snap.
    state_->podRoll += (input.steer * 0.45f - state_->podRoll) *
                       std::min(1.f, 6.f * dt);
    if (state_->pod.airTime > kFallSeconds) {
        if (logger_) logger_->Trace("racer.pod.drive: off course, respawn");
        PlaceRacerPodOnLap(*state_, std::max(0, state_->race.segment),
                           kRespawnSpeedShare * state_->podSpec.topSpeed);
    }
    Publish(context, *state_);
}

}  // namespace sdl3cpp::services::impl

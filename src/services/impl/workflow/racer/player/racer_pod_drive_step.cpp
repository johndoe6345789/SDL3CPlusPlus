#include "services/interfaces/workflow/racer/player/racer_pod_drive_step.hpp"

#include "services/interfaces/workflow/racer/player/racer_line_guide.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMaxDt = 1.f / 30.f;   // a long frame must not tunnel
constexpr float kRespawnSpeedShare = 0.3f;

}  // namespace

WorkflowRacerPodDriveStep::WorkflowRacerPodDriveStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerPodDriveStep::GetPluginId() const {
    return "racer.pod.drive";
}

void WorkflowRacerPodDriveStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    if (!state_->loaded) return;
    const RacerPhase phase = state_->flow.phase;
    if (phase != RacerPhase::Racing && phase != RacerPhase::Results) {
        return;
    }
    const float dt = std::min(
        kMaxDt,
        static_cast<float>(context.Get<double>("frame.delta_time", 0.0)));
    const RacerPodInput input = ReadRacerPodInput(step, context, *state_);
    StepRacerPod(state_->pod, input, state_->podSpec, dt,
                 RacerGroundSurface(state_->ground));
    if (RacerPodOnAutopilot(step, *state_)) {
        GuideRacerPodToLine(state_->pod, state_->lapPoints,
                            state_->race.segment, kRacerAiLineOffset,
                            kRacerAiLinePull, dt);
    }
    // Bank into the turn, easing so the pod does not snap.
    state_->podRoll +=
        (input.steer * 0.45f - state_->podRoll) * std::min(1.f, 6.f * dt);
    RacerPodState& pod = state_->pod;
    const bool crawling = input.throttle > 0.5f && pod.speed < 3.f;
    pod.stuckTime = (pod.blocked || crawling) ? pod.stuckTime + dt : 0.f;
    const RacerRecoveryReason reason =
        UpdateRacerRecovery(recovery_, pod, state_->race, state_->ground,
                            state_->lapPoints, input.throttle, dt);
    if (reason != RacerRecoveryReason::None) {
        TraceRacerRespawn(logger_, *state_, RacerRecoveryName(reason));
        const int point = RacerRespawnPoint(
            recovery_, state_->race.segment, reason, kRacerLapSamples);
        PlaceRacerPodOnLap(*state_, point,
                           kRespawnSpeedShare * state_->podSpec.topSpeed);
    }
    traceClock_ += dt;
    if (traceClock_ >= 0.25f) {
        traceClock_ = 0.f;
        TraceRacerPod(logger_, *state_);
    }
    PublishRacerPod(context, *state_);
}

}  // namespace sdl3cpp::services::impl

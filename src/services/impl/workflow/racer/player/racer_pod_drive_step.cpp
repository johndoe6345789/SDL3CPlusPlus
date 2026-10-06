#include "services/interfaces/workflow/racer/player/racer_pod_drive_step.hpp"

#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_report.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_build.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kMaxDt = 1.f / 30.f;   // a long frame must not tunnel
constexpr float kRespawnSpeedShare = 0.3f;

std::optional<float> Height(const void* ground, float x, float z,
                            float ceiling) {
    return RacerGroundHeight(*static_cast<const RacerGround*>(ground), x, z,
                             ceiling);
}

bool Wall(const void* ground, const glm::vec3& from, const glm::vec3& to) {
    return RacerWallBetween(*static_cast<const RacerGround*>(ground), from,
                            to);
}

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
    const float dt = std::min(
        kMaxDt,
        static_cast<float>(context.Get<double>("frame.delta_time", 0.0)));
    const RacerPodInput input = ReadRacerPodInput(step, context, *state_);
    StepRacerPod(state_->pod, input, state_->podSpec, dt,
                 RacerSurface{&Height, &Wall, &state_->ground});
    // Bank into the turn, easing so the pod does not snap.
    state_->podRoll +=
        (input.steer * 0.45f - state_->podRoll) * std::min(1.f, 6.f * dt);
    RacerPodState& pod = state_->pod;
    const bool crawling = input.throttle > 0.5f && pod.speed < 3.f;
    pod.stuckTime = (pod.blocked || crawling) ? pod.stuckTime + dt : 0.f;
    const RacerRecoveryReason reason =
        UpdateRacerRecovery(recovery_, *state_, input.throttle, dt);
    if (reason != RacerRecoveryReason::None) {
        if (logger_) {
            logger_->Trace(std::string("racer.pod.drive: ") +
                           RacerRecoveryName(reason) + ", respawn");
        }
        // Lost for want of progress: put back a little further on.
        const int ahead = reason == RacerRecoveryReason::NoProgress
                              ? 2 * kRacerLapSamples
                              : 0;
        PlaceRacerPodOnLap(*state_, std::max(0, state_->race.segment) + ahead,
                           kRespawnSpeedShare * state_->podSpec.topSpeed);
    }
    traceClock_ += dt;
    if (traceClock_ >= 1.f) {
        traceClock_ = 0.f;
        TraceRacerPod(logger_, *state_);
    }
    PublishRacerPod(context, *state_);
}

}  // namespace sdl3cpp::services::impl

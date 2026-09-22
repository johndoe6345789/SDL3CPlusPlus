#include "services/interfaces/workflow/stunts/player/stunts_lap_timer_step.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// True while the car is still over the cell it started on.
bool OnStartCell(const StuntsWorldState& state, const glm::vec3& at) {
    const float half = state.params.tileSize * 0.5f;
    return std::fabs(at.x - state.start.position.x) <= half &&
           std::fabs(at.z - state.start.position.z) <= half;
}

}  // namespace

WorkflowStuntsLapTimerStep::WorkflowStuntsLapTimerStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<StuntsWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowStuntsLapTimerStep::GetPluginId() const {
    return "stunts.lap.timer";
}

void WorkflowStuntsLapTimerStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    (void)step;
    if (!state_->loaded || !state_->start.found) return;
    const auto* at = context.TryGet<glm::vec3>("stunts.car_pos");
    if (!at) return;

    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    const bool onStart = OnStartCell(*state_, *at);
    if (!running_ && !onStart) {
        running_ = true;
        away_ = true;
        lapTime_ = 0.f;
    } else if (running_) {
        lapTime_ += dt;
        if (!onStart) {
            away_ = true;
        } else if (away_) {
            ++lap_;
            if (bestLap_ <= 0.f || lapTime_ < bestLap_) bestLap_ = lapTime_;
            if (logger_) {
                logger_->Info("stunts.lap.timer: lap " + std::to_string(lap_) +
                              " in " + std::to_string(lapTime_) + "s");
            }
            lapTime_ = 0.f;
            away_ = false;
        }
    }

    context.Set("stunts.lap", lap_);
    context.Set("stunts.lap_time", lapTime_);
    context.Set("stunts.best_lap", bestLap_);
}

}  // namespace sdl3cpp::services::impl

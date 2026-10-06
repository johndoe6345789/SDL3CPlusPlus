#include "services/interfaces/workflow/racer/player/racer_lap_timer_step.hpp"

#include "services/interfaces/workflow/racer/player/racer_lap_progress.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRacerLapTimerStep::WorkflowRacerLapTimerStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerLapTimerStep::GetPluginId() const {
    return "racer.lap.timer";
}

void WorkflowRacerLapTimerStep::Execute(const WorkflowStepDefinition&,
                                        WorkflowContext& context) {
    if (!state_->loaded) return;
    const RacerPhase phase = state_->flow.phase;
    if (phase != RacerPhase::Racing && phase != RacerPhase::Results) {
        return;
    }
    RacerRaceState& race = state_->race;
    const float dt =
        static_cast<float>(context.Get<double>("frame.delta_time", 0.0));
    const int point = NearestRacerLapPoint(
        state_->lapPoints, state_->pod.position, race.segment);
    const int lapBefore = race.lap;
    AdvanceRacerRace(race, point,
                     static_cast<int>(state_->lapPoints.size()), dt);
    if (race.lap != lapBefore && logger_) {
        logger_->Info("racer.lap.timer: lap " + std::to_string(lapBefore) +
                      " done, best " + std::to_string(race.bestLap) + " s");
    }
    context.Set("racer.lap", race.lap);
    context.Set("racer.laps", race.lapsTotal);
    context.Set("racer.race_time", race.raceTime);
    context.Set("racer.lap_time", race.lapTime);
    context.Set("racer.best_lap", race.bestLap);
    context.Set("racer.countdown", race.countdown);
    context.Set("racer.finished", race.finished);
}

}  // namespace sdl3cpp::services::impl

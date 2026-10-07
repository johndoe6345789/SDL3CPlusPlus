#include "services/interfaces/workflow/racer/player/racer_opponents_step.hpp"

#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRacerOpponentsStep::WorkflowRacerOpponentsStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerOpponentsStep::GetPluginId() const {
    return "racer.opponents.fly";
}

void WorkflowRacerOpponentsStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    if (!state_->loaded) return;
    const RacerPhase phase = state_->flow.phase;
    if (phase != RacerPhase::Racing && phase != RacerPhase::Results) {
        return;
    }
    const float dt = std::min(
        1.f / 30.f,
        static_cast<float>(context.Get<double>("frame.delta_time", 0.0)));
    std::vector<RacerPodState*> pods{&state_->pod};
    for (RacerOpponent& opponent : state_->opponents) {
        FlyRacerOpponent(*state_, opponent, dt);
        pods.push_back(&opponent.pod);
    }
    for (RacerPodState* pod : pods) pod->bumped = false;
    SeparateRacerPods(pods);
    if (state_->race.countdown <= 0.f) {
        state_->hazardSounds |= UpdateRacerHazards(state_->hazards, pods, dt);
    }
    RankRacerField(*state_);
    context.Set("racer.position", state_->race.position);
    context.Set("racer.entrants", state_->race.entrants);
}

}  // namespace sdl3cpp::services::impl

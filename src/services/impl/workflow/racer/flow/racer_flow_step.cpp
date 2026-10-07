#include "services/interfaces/workflow/racer/flow/racer_flow_step.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <algorithm>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

/// True on the frame `now` turns on.
bool Edge(bool now, bool& was) {
    const bool pressed = now && !was;
    was = now;
    return pressed;
}

}  // namespace

RacerNav RacerNavReader::Read(const WorkflowContext& context) {
    const float forward = context.Get<float>("input.move_forward", 0.f);
    const float right = context.Get<float>("input.move_right", 0.f);
    RacerNav nav;
    nav.up = Edge(forward > 0.5f, up_);
    nav.down = Edge(forward < -0.5f, down_);
    nav.right = Edge(right > 0.5f, right_);
    nav.left = Edge(right < -0.5f, left_);
    nav.select =
        Edge(context.GetBool("racer.select_pressed", false), select_);
    nav.back = Edge(context.GetBool("racer.back_pressed", false), back_);
    return nav;
}

WorkflowRacerFlowStep::WorkflowRacerFlowStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowRacerFlowStep::GetPluginId() const {
    return "racer.flow";
}

void WorkflowRacerFlowStep::Execute(const WorkflowStepDefinition&,
                                    WorkflowContext& context) {
    RacerFlow& flow = state_->flow;
    const RacerNav nav = nav_.Read(context);
    const float dt = std::min(
        0.1f, static_cast<float>(context.Get<double>("frame.delta_time", 0.0)));
    const RacerPhase before = flow.phase;
    if (flow.initialised) {
        switch (flow.phase) {
        case RacerPhase::Menu: UpdateRacerMenu(flow, nav, state_->table); break;
        case RacerPhase::Tournament:
            UpdateRacerTournament(flow, nav, state_->table);
            break;
        case RacerPhase::FreeRace:
            UpdateRacerFreeRace(flow, nav, state_->table);
            break;
        case RacerPhase::Shop: UpdateRacerShop(flow, nav); break;
        case RacerPhase::Junkyard: UpdateRacerJunkyard(flow, nav); break;
        case RacerPhase::PitDroids: UpdateRacerPitDroids(flow, nav); break;
        case RacerPhase::Loading: break;
        case RacerPhase::Cutscene: flow.skipVideo = nav.select || nav.back;
            break;
        default: UpdateRacerRaceFlow(flow, *state_, nav, dt); break;
        }
    }
    if (flow.phase != before && logger_) {
        logger_->Trace("racer.flow: phase " +
                       std::to_string(static_cast<int>(before)) + " -> " +
                       std::to_string(static_cast<int>(flow.phase)));
    }
    context.Set("racer.quit_requested", flow.quit);
    context.Set("racer.phase", static_cast<int>(flow.phase));
}

}  // namespace sdl3cpp::services::impl

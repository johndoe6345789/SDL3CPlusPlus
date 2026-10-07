#include "services/interfaces/workflow/racer/render/racer_screen_step.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

namespace sdl3cpp::services::impl {

RacerScreenContent WorkflowRacerScreenStep::Content() const {
    const RacerFlow& flow = state_->flow;
    RacerScreenContent screen;
    switch (flow.phase) {
    case RacerPhase::Menu:
        screen = {&title_, RacerMenuLines(flow, state_->table), 110};
        break;
    case RacerPhase::Loading:
        screen = {&title_, RacerLoadingLines(flow, state_->table), 150};
        break;
    case RacerPhase::Tournament:
        screen = {&title_, RacerTournamentLines(flow, state_->table), 120};
        break;
    case RacerPhase::FreeRace:
        screen = {&title_, RacerFreeRaceLines(flow, state_->table), 120};
        break;
    case RacerPhase::Shop:
        screen = {&hangar_, RacerShopLines(flow), 130};
        break;
    case RacerPhase::Junkyard:
        screen = {&junkyard_, RacerJunkyardLines(flow), 130};
        break;
    case RacerPhase::PitDroids:
        screen = {&hangar_, RacerPitDroidLines(flow), 130};
        break;
    case RacerPhase::Cutscene:
        screen = {nullptr, {}, 255};  // black; the video plays on top
        break;
    case RacerPhase::Paused:
        screen = {nullptr, RacerPauseLines(flow), 150};
        break;
    default:
        screen = {nullptr, RacerResultsLines(flow, *state_), 170};
        break;
    }
    return screen;
}

}  // namespace sdl3cpp::services::impl

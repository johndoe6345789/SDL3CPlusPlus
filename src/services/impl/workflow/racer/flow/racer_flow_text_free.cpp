#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

}  // namespace

std::vector<RacerPanelLine> RacerFreeRaceLines(const RacerFlow& flow,
                                               const RacerTrackTable& table) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerBand(14.f, 256.f));
    lines.push_back(RacerCentred("FREE RACE", 22.f, kGold));
    std::string track = "-";
    if (flow.trackIndex < static_cast<int>(table.tracks.size())) {
        const auto& t = table.tracks[flow.trackIndex];
        track = RacerUpper(t.name) + " (" + RacerUpper(t.planet) + ")";
    }
    RacerRowLines(lines,
                  {"TRACK  < " + track + " >",
                   "RACER  < " + RacerChosenRacer(flow, table) + " >",
                   "LAPS  < " + std::to_string(flow.laps) + " >",
                   "RIVALS  < " + std::to_string(flow.opponents) + " >",
                   "START RACE", "BACK"},
                  flow.setupRow, 56.f, 22.f);
    lines.push_back(RacerCentred("TRACKS AND RACERS OPEN AS YOU WIN", 200.f,
                                 kGrey));
    lines.push_back(RacerCentred("A FREE RACE PAYS NOTHING", 216.f, kGrey));
    lines.push_back(RacerCentred("ARROWS CHOOSE   ENTER GO   ESC BACK",
                                 242.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

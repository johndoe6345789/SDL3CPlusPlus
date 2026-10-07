#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"
#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

}  // namespace

std::vector<RacerPanelLine> RacerTournamentLines(
    const RacerFlow& flow, const RacerTrackTable& table) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerBand(14.f, 256.f));
    lines.push_back(RacerCentred("TOURNAMENT", 22.f, kGold));
    const int c = flow.circuit, slot = flow.circuitTrack;
    const int best = flow.profile.best[c * kRacerCircuitTracks + slot];
    std::string track = RacerUpper(RacerCircuitTrackName(c, slot));
    if (best > 0) track += "  (BEST " + RacerPlaceText(best) + ")";
    RacerRowLines(lines,
                  {"CIRCUIT  < " + std::string(RacerCircuitName(c)) + " >",
                   "TRACK  < " + track + " >",
                   "RACER  < " + RacerChosenRacer(flow, table) + " >",
                   "WINNINGS  < " + std::string(RacerSplitName(flow.split)) +
                       " >",
                   "START RACE", "BACK"},
                  flow.setupRow, 46.f, 20.f);
    std::string purse = "PURSE ";
    for (int place = 1; place <= 4; ++place) {
        const int prize = RacerPurseFor(c, flow.split, place);
        if (prize > 0) {
            purse += " " + RacerPlaceText(place) + " " + std::to_string(prize);
        }
    }
    lines.push_back(RacerCentred(purse, 178.f, kGold));
    lines.push_back(RacerCentred(
        "TRACK " + std::to_string(slot + 1) + " OF " +
            std::to_string(RacerCircuitTrackCount(c)) + "   POINTS " +
            std::to_string(flow.profile.points[c]) + "   TRUGUTS " +
            std::to_string(flow.profile.truguts),
        198.f, kGrey));
    lines.push_back(RacerCentred("A PODIUM OPENS THE NEXT TRACK", 222.f,
                                 kGrey));
    lines.push_back(RacerCentred("ARROWS CHOOSE   ENTER GO   ESC BACK",
                                 242.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <algorithm>
#include <cctype>

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

void Title(std::vector<RacerPanelLine>& lines, const std::string& below) {
    lines.push_back(RacerBand(12.f, 62.f));
    lines.push_back(RacerCentred("STAR WARS  EPISODE I", 20.f, kGold));
    lines.push_back(RacerCentred("R  A  C  E  R", 34.f, kGold));
    lines.push_back(RacerCentred(below, 50.f, kGrey));
}

}  // namespace

std::vector<RacerPanelLine> RacerMenuLines(const RacerFlow& flow,
                                           const RacerTrackTable& table) {
    std::vector<RacerPanelLine> lines;
    Title(lines, "2026 EDITION");
    std::string track = "-", racer = "-";
    if (flow.trackIndex < static_cast<int>(table.tracks.size())) {
        const auto& t = table.tracks[flow.trackIndex];
        track = RacerUpper(t.name) + " (" + RacerUpper(t.planet) + ")";
    }
    if (flow.racerIndex < static_cast<int>(table.racers.size())) {
        racer = RacerUpper(table.racers[flow.racerIndex].name);
    }
    const std::string rows[] = {
        "TRACK   < " + track + " >", "RACER   < " + racer + " >",
        "LAPS    < " + std::to_string(flow.laps) + " >",
        "RIVALS  < " + std::to_string(flow.opponents) + " >", "POD SHOP",
        "START RACE", "QUIT"};
    lines.push_back(RacerBand(80.f, 216.f));
    lines.push_back(RacerBand(220.f, 258.f));
    for (int i = 0; i < 7; ++i) {
        const bool on = flow.menuRow == i;
        lines.push_back(RacerCentred((on ? "> " : "  ") + rows[i],
                                     90.f + 18.f * i, RacerRowColour(on)));
    }
    lines.push_back(RacerCentred(
        "TRUGUTS " + std::to_string(flow.profile.truguts), 226.f, kGold));
    lines.push_back(RacerCentred(
        "ARROWS CHOOSE   ENTER GO   ESC QUIT", 246.f, kGrey));
    return lines;
}

std::vector<RacerPanelLine> RacerLoadingLines(const RacerFlow& flow,
                                              const RacerTrackTable& table) {
    std::vector<RacerPanelLine> lines;
    Title(lines, "");
    std::string track = "";
    if (flow.trackIndex < static_cast<int>(table.tracks.size())) {
        track = RacerUpper(table.tracks[flow.trackIndex].name);
    }
    lines.push_back(RacerCentred(track, 120.f, kGold));
    lines.push_back(RacerCentred("LOADING ...", 150.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

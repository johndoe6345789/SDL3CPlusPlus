#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

}  // namespace

void RacerTitleLines(std::vector<RacerPanelLine>& lines,
                     const std::string& below) {
    lines.push_back(RacerBand(12.f, 62.f));
    lines.push_back(RacerCentred("STAR WARS  EPISODE I", 20.f, kGold));
    lines.push_back(RacerCentred("R  A  C  E  R", 34.f, kGold));
    lines.push_back(RacerCentred(below, 50.f, kGrey));
}

void RacerRowLines(std::vector<RacerPanelLine>& lines,
                   const std::vector<std::string>& rows, int selected,
                   float top, float spacing) {
    for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
        const bool on = selected == i;
        lines.push_back(RacerCentred((on ? "> " : "  ") + rows[i],
                                     top + spacing * i, RacerRowColour(on)));
    }
}

std::vector<RacerPanelLine> RacerMenuLines(const RacerFlow& flow,
                                           const RacerTrackTable&) {
    std::vector<RacerPanelLine> lines;
    RacerTitleLines(lines, "2026 EDITION");
    lines.push_back(RacerBand(80.f, 216.f));
    lines.push_back(RacerBand(220.f, 258.f));
    RacerRowLines(lines,
                  {"TOURNAMENT", "FREE RACE", "WATTO'S SHOP", "JUNKYARD",
                   "PIT DROIDS", "QUIT"},
                  flow.menuRow, 96.f, 19.f);
    lines.push_back(RacerCentred(
        "TRUGUTS " + std::to_string(flow.profile.truguts), 226.f, kGold));
    lines.push_back(RacerCentred(
        flow.notice.empty() ? "ARROWS CHOOSE   ENTER GO   ESC QUIT"
                            : flow.notice,
        246.f, kGrey));
    return lines;
}

std::vector<RacerPanelLine> RacerLoadingLines(const RacerFlow& flow,
                                              const RacerTrackTable& table) {
    std::vector<RacerPanelLine> lines;
    RacerTitleLines(lines, "");
    std::string track = "";
    if (flow.trackIndex < static_cast<int>(table.tracks.size())) {
        const auto& t = table.tracks[flow.trackIndex];
        track = RacerUpper(t.name) + " - " + RacerUpper(t.planet);
    }
    lines.push_back(RacerBand(108.f, 168.f));
    lines.push_back(RacerCentred(track, 120.f, kGold));
    lines.push_back(RacerCentred("LOADING ...", 150.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <cmath>
#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

}  // namespace

std::vector<RacerPanelLine> RacerJunkyardLines(const RacerFlow& flow) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerBand(14.f, 258.f));
    lines.push_back(RacerCentred("WATTO'S JUNKYARD", 22.f, kGold));
    lines.push_back(RacerCentred(
        "TRUGUTS " + std::to_string(flow.profile.truguts), 38.f, kGold));
    std::vector<std::string> rows;
    for (const RacerJunkOffer& offer : flow.junk) {
        char text[64];
        std::snprintf(text, sizeof(text), "%-20s %3d%% %6d",
                      RacerUpper(RacerPart(offer.type, offer.level).name)
                          .c_str(),
                      static_cast<int>(std::round(100.f * offer.health)),
                      offer.price);
        rows.push_back(text);
    }
    rows.push_back("BACK");
    RacerRowLines(lines, rows, flow.shopRow, 64.f, 20.f);
    lines.push_back(RacerCentred("USED PARTS: CHEAP, AND WORN", 196.f, kGrey));
    lines.push_back(RacerCentred("NEW STOCK AFTER EVERY TOURNAMENT RACE",
                                 210.f, kGrey));
    lines.push_back(RacerCentred(flow.notice, 228.f, kGold));
    lines.push_back(RacerCentred("ENTER BUY AND FIT   ESC BACK", 246.f,
                                 kGrey));
    return lines;
}

std::vector<RacerPanelLine> RacerPitDroidLines(const RacerFlow& flow) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerBand(14.f, 258.f));
    lines.push_back(RacerCentred("PIT DROIDS", 22.f, kGold));
    const int droids = flow.profile.pitDroids;
    lines.push_back(RacerCentred(
        "CREW " + std::to_string(droids) + " OF " +
            std::to_string(kRacerMaxPitDroids) + "   TRUGUTS " +
            std::to_string(flow.profile.truguts),
        38.f, kGold));
    for (int type = 0; type < kRacerUpgradeCount; ++type) {
        lines.push_back(RacerCentred(
            RacerFittedPartRow(flow.profile, type), 58.f + 14.f * type, kGrey));
    }
    RacerRowLines(lines,
                  {"BUY A PIT DROID  " + std::to_string(kRacerPitDroidPrice),
                   "BACK"},
                  flow.shopRow, 164.f, 18.f);
    lines.push_back(RacerCentred("AFTER EACH RACE EVERY DROID MENDS 6% OF",
                                 202.f, kGrey));
    lines.push_back(RacerCentred("EACH PART", 214.f, kGrey));
    lines.push_back(RacerCentred(flow.notice, 230.f, kGold));
    lines.push_back(RacerCentred("ENTER BUY   ESC BACK", 246.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

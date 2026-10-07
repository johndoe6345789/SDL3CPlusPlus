#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <cmath>
#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

int Percent(float health) {
    return static_cast<int>(std::round(100.f * health));
}

}  // namespace

std::string RacerFittedPartRow(const RacerProfile& profile, int type) {
    char text[64];
    std::snprintf(text, sizeof(text), "%-12s %-20s %3d%%",
                  RacerUpgradeName(type),
                  RacerUpper(RacerPart(type, profile.upgrades[type]).name)
                      .c_str(),
                  Percent(profile.health[type]));
    return text;
}

std::vector<RacerPanelLine> RacerShopLines(const RacerFlow& flow) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerBand(14.f, 258.f));
    lines.push_back(RacerCentred("WATTO'S SHOP", 22.f, kGold));
    lines.push_back(RacerCentred(
        "TRUGUTS " + std::to_string(flow.profile.truguts), 38.f, kGold));
    std::vector<std::string> rows;
    for (int type = 0; type < kRacerUpgradeCount; ++type) {
        rows.push_back(RacerFittedPartRow(flow.profile, type));
    }
    rows.push_back("BACK");
    RacerRowLines(lines, rows, flow.shopRow, 60.f, 17.f);
    if (flow.shopRow < kRacerUpgradeCount) {
        const int type = flow.shopRow;
        const RacerPartInfo& part = RacerPart(type, flow.shopLevel);
        const int tradeIn = RacerTradeInValue(
            type, flow.profile.upgrades[type], flow.profile.health[type]);
        lines.push_back(RacerCentred(
            "< " + RacerUpper(part.name) + " >  " +
                std::to_string(part.price) + " TRUGUTS",
            200.f, RacerRowColour(true)));
        lines.push_back(RacerCentred(
            "TRADE-IN " + std::to_string(tradeIn) +
                "   MORE STOCK AS YOU PLACE",
            214.f, kGrey));
    }
    lines.push_back(RacerCentred(flow.notice, 230.f, kGold));
    lines.push_back(RacerCentred("LEFT/RIGHT PART   ENTER BUY   ESC BACK",
                                 246.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

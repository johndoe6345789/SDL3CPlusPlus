#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

constexpr SDL_Color kGold{255, 205, 70, 255};
constexpr SDL_Color kGrey{200, 200, 210, 255};

std::string Bar(int level) {
    return "[" + std::string(level, '#') +
           std::string(kRacerUpgradeMax - level, '-') + "]";
}

}  // namespace

std::vector<RacerPanelLine> RacerShopLines(const RacerFlow& flow) {
    std::vector<RacerPanelLine> lines;
    lines.push_back(RacerCentred("WATTO'S POD SHOP", 24.f, kGold));
    lines.push_back(RacerCentred(
        "TRUGUTS " + std::to_string(flow.profile.truguts), 42.f, kGold));
    for (int i = 0; i < kRacerUpgradeCount; ++i) {
        const int level = flow.profile.upgrades[i];
        const std::string cost =
            level >= kRacerUpgradeMax
                ? std::string("  MAX")
                : std::to_string(RacerUpgradeCost(level));
        char text[64];
        std::snprintf(text, sizeof(text), "%s%-13s %s %5s",
                      flow.shopRow == i ? "> " : "  ", RacerUpgradeName(i),
                      Bar(level).c_str(), cost.c_str());
        lines.push_back(RacerCentred(text, 74.f + 18.f * i,
                                     RacerRowColour(flow.shopRow == i)));
    }
    const bool back = flow.shopRow == kRacerUpgradeCount;
    lines.push_back(RacerCentred(back ? "> BACK" : "  BACK", 206.f,
                                 RacerRowColour(back)));
    lines.push_back(RacerCentred(flow.notice, 228.f, kGold));
    lines.push_back(RacerCentred("ENTER BUY   ESC BACK", 248.f, kGrey));
    return lines;
}

}  // namespace sdl3cpp::services::impl

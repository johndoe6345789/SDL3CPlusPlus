#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

enum MenuRow { kTrack, kRacer, kLaps, kRivals, kShop, kStart, kQuit, kRows };

int Wrap(int value, int count) {
    return count > 0 ? ((value % count) + count) % count : 0;
}

}  // namespace

void UpdateRacerMenu(RacerFlow& flow, const RacerNav& nav,
                     const RacerTrackTable& table) {
    if (nav.up) flow.menuRow = Wrap(flow.menuRow - 1, kRows);
    if (nav.down) flow.menuRow = Wrap(flow.menuRow + 1, kRows);
    const int step = (nav.right ? 1 : 0) - (nav.left ? 1 : 0);
    const int tracks = static_cast<int>(table.tracks.size());
    const int racers = static_cast<int>(table.racers.size());
    switch (flow.menuRow) {
    case kTrack: flow.trackIndex = Wrap(flow.trackIndex + step, tracks); break;
    case kRacer: flow.racerIndex = Wrap(flow.racerIndex + step, racers); break;
    case kLaps: flow.laps = std::clamp(flow.laps + step, 1, 5); break;
    case kRivals:
        flow.opponents = std::clamp(flow.opponents + step, 0, 11);
        break;
    default: break;
    }
    if (nav.back) flow.quit = true;
    if (!nav.select) return;
    if (flow.menuRow == kShop) {
        flow.phase = RacerPhase::Shop;
        flow.shopRow = 0;
        flow.notice.clear();
    } else if (flow.menuRow == kStart || flow.menuRow < kShop) {
        flow.phase = RacerPhase::Loading;
        flow.requestLoad = true;
        flow.loadingFrames = 0;
    } else if (flow.menuRow == kQuit) {
        flow.quit = true;
    }
}

void UpdateRacerShop(RacerFlow& flow, const RacerNav& nav) {
    const int rows = kRacerUpgradeCount + 1;  // the parts, then BACK
    if (nav.up) flow.shopRow = Wrap(flow.shopRow - 1, rows);
    if (nav.down) flow.shopRow = Wrap(flow.shopRow + 1, rows);
    const bool leave = nav.back || (nav.select && flow.shopRow == rows - 1);
    if (leave) {
        flow.phase = RacerPhase::Menu;
        flow.notice.clear();
        return;
    }
    if (!nav.select) return;
    int& level = flow.profile.upgrades[flow.shopRow];
    const int cost = RacerUpgradeCost(level);
    if (level >= kRacerUpgradeMax) {
        flow.notice = "THAT PART IS FULLY UPGRADED";
    } else if (flow.profile.truguts < cost) {
        flow.notice = "NOT ENOUGH TRUGUTS - WIN SOME RACES";
    } else {
        flow.profile.truguts -= cost;
        ++level;
        flow.notice = std::string("BOUGHT ") +
                      RacerUpgradeName(flow.shopRow) + " LEVEL " +
                      std::to_string(level);
        SaveRacerProfile(flow.profile, RacerProfilePath());
    }
}

}  // namespace sdl3cpp::services::impl

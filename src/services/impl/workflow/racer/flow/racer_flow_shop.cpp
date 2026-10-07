#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

/// Rows: the seven part types, then BACK.
constexpr int kShopRows = kRacerUpgradeCount + 1;

/// On a new row, look first at the part one better than the fitted one.
void LookAtNext(RacerFlow& flow) {
    if (flow.shopRow >= kRacerUpgradeCount) return;
    const int fitted = flow.profile.upgrades[flow.shopRow];
    const int stocked = RacerStockedLevel(flow.shopRow, flow.profile.podiums);
    flow.shopLevel = std::min(fitted + 1, stocked);
}

}  // namespace

void UpdateRacerShop(RacerFlow& flow, const RacerNav& nav) {
    if (nav.up || nav.down) {
        flow.shopRow = (flow.shopRow + (nav.up ? kShopRows - 1 : 1)) %
                       kShopRows;
        LookAtNext(flow);
        flow.notice.clear();
    }
    const bool back = flow.shopRow == kRacerUpgradeCount;
    if (nav.back || (nav.select && back)) {
        flow.phase = RacerPhase::Menu;
        flow.notice.clear();
        return;
    }
    if (back) return;
    const int type = flow.shopRow;
    const int top = RacerStockedLevel(type, flow.profile.podiums);
    const int step = (nav.right ? 1 : 0) - (nav.left ? 1 : 0);
    flow.shopLevel = std::clamp(flow.shopLevel + step, 0, top);
    if (!nav.select) return;
    const bool fitted = flow.profile.upgrades[type] == flow.shopLevel &&
                        flow.profile.health[type] >= 1.f;
    if (fitted) {
        flow.notice = "THAT PART IS FITTED ALREADY";
        return;
    }
    FitRacerPart(flow, type, flow.shopLevel, 1.f,
                 RacerPart(type, flow.shopLevel).price);
}

}  // namespace sdl3cpp::services::impl

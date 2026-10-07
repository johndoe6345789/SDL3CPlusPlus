#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void UpdateRacerJunkyard(RacerFlow& flow, const RacerNav& nav) {
    // A fresh pile after every tournament race.
    if (flow.junkDrawnAt != flow.profile.racesRun) {
        flow.junk = RacerJunkyardStock(flow.profile);
        flow.junkDrawnAt = flow.profile.racesRun;
    }
    const int rows = static_cast<int>(flow.junk.size()) + 1;  // + BACK
    if (flow.shopRow >= rows) flow.shopRow = rows - 1;
    if (nav.up) flow.shopRow = (flow.shopRow + rows - 1) % rows;
    if (nav.down) flow.shopRow = (flow.shopRow + 1) % rows;
    const bool back = flow.shopRow == rows - 1;
    if (nav.back || (nav.select && back)) {
        flow.phase = RacerPhase::Menu;
        flow.notice.clear();
        return;
    }
    if (!nav.select) return;
    const RacerJunkOffer offer = flow.junk[flow.shopRow];
    if (FitRacerPart(flow, offer.type, offer.level, offer.health,
                     offer.price)) {
        flow.junk.erase(flow.junk.begin() + flow.shopRow);
    }
}

void UpdateRacerPitDroids(RacerFlow& flow, const RacerNav& nav) {
    if (nav.up || nav.down) flow.shopRow = 1 - std::clamp(flow.shopRow, 0, 1);
    if (nav.back || (nav.select && flow.shopRow == 1)) {
        flow.phase = RacerPhase::Menu;
        flow.notice.clear();
        return;
    }
    if (!nav.select) return;
    RacerProfile& profile = flow.profile;
    if (profile.pitDroids >= kRacerMaxPitDroids) {
        flow.notice = "YOUR PIT CREW IS FULL";
    } else if (profile.truguts < kRacerPitDroidPrice) {
        flow.notice = "NOT ENOUGH TRUGUTS - WIN SOME RACES";
    } else {
        profile.truguts -= kRacerPitDroidPrice;
        ++profile.pitDroids;
        flow.notice = "A NEW PIT DROID JOINS THE CREW";
        SaveRacerProfile(profile, RacerProfilePath());
    }
}

}  // namespace sdl3cpp::services::impl

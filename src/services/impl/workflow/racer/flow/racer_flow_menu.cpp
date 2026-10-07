#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

namespace sdl3cpp::services::impl {
namespace {

enum TitleRow { kTournament, kFreeRace, kShop, kJunk, kDroids, kQuit, kRows };

int Wrap(int value, int count) {
    return count > 0 ? ((value % count) + count) % count : 0;
}

}  // namespace

int RacerStepOpen(int index, int step, int count,
                  const std::vector<bool>& open) {
    for (int tries = 0; tries < count; ++tries) {
        index = Wrap(index + (step == 0 ? 1 : step), count);
        if (index < static_cast<int>(open.size()) && open[index]) break;
    }
    return index;
}

bool RacerTrackOpen(const RacerFlow& flow, const RacerTrackTable& table,
                    int index) {
    if (flow.unlockAll) return true;
    const RacerProfile& profile = flow.profile;
    for (int c = 0; c < profile.circuitsOpen && c < kRacerCircuits; ++c) {
        for (int slot = 0; slot < profile.tracksOpen[c]; ++slot) {
            if (RacerCircuitTrackIndex(table, c, slot) == index) return true;
        }
    }
    return false;
}

bool RacerRacerOpen(const RacerFlow& flow, int index) {
    return flow.unlockAll ||
           (index >= 0 && index < 32 && (flow.profile.racers >> index) & 1u);
}

void UpdateRacerMenu(RacerFlow& flow, const RacerNav& nav,
                     const RacerTrackTable&) {
    if (nav.up) flow.menuRow = Wrap(flow.menuRow - 1, kRows);
    if (nav.down) flow.menuRow = Wrap(flow.menuRow + 1, kRows);
    if (nav.back) flow.quit = true;
    if (!nav.select) return;
    flow.notice.clear();
    flow.setupRow = 0;
    flow.shopRow = 0;
    switch (flow.menuRow) {
    case kTournament: flow.phase = RacerPhase::Tournament; break;
    case kFreeRace: flow.phase = RacerPhase::FreeRace; break;
    case kShop: flow.phase = RacerPhase::Shop; break;
    case kJunk: flow.phase = RacerPhase::Junkyard; break;
    case kDroids: flow.phase = RacerPhase::PitDroids; break;
    default: flow.quit = true; break;
    }
}

}  // namespace sdl3cpp::services::impl

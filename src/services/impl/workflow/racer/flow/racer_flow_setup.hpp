#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// Rows shared by the tournament and free-race set-up screens.
inline constexpr int kSetupRows = 6;

/// Off to the loading screen, then the race.
inline void StartRacerLoad(RacerFlow& flow) {
    flow.phase = RacerPhase::Loading;
    flow.requestLoad = true;
    flow.loadingFrames = 0;
}

inline std::vector<bool> OpenRacerList(const RacerFlow& flow, int count) {
    std::vector<bool> open(count);
    for (int i = 0; i < count; ++i) open[i] = RacerRacerOpen(flow, i);
    return open;
}

/// Up and down through the rows; back leaves for the title.
inline int MoveSetupRow(RacerFlow& flow, const RacerNav& nav) {
    if (nav.up) flow.setupRow = (flow.setupRow + kSetupRows - 1) % kSetupRows;
    if (nav.down) flow.setupRow = (flow.setupRow + 1) % kSetupRows;
    if (nav.back) flow.phase = RacerPhase::Menu;
    return (nav.right ? 1 : 0) - (nav.left ? 1 : 0);
}

}  // namespace sdl3cpp::services::impl

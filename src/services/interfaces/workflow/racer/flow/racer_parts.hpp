#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// One of Watto's parts, from the game's own parts table: its name, its
/// price new, and how many podium finishes before Watto stocks it.
struct RacerPartInfo {
    const char* name;
    int price;
    int podiumsNeeded;
};

/// The part of `level` (0..5) in part type `type` (0..6).
const RacerPartInfo& RacerPart(int type, int level);

/// The best part of `type` Watto stocks after `podiums` podium finishes.
int RacerStockedLevel(int type, int podiums);

/// The part type's name: TRACTION, TURNING, ... REPAIR.
const char* RacerUpgradeName(int type);

/// What Watto allows for the fitted part when another replaces it:
/// a quarter of its price, less for wear.
int RacerTradeInValue(int type, int level, float health);

/// The junkyard's stock after `racesRun` races: it changes each race,
/// and only holds parts Watto would sell by now.
std::vector<RacerJunkOffer> RacerJunkyardStock(const RacerProfile& profile);

/// What a pit droid costs.
inline constexpr int kRacerPitDroidPrice = 2000;

/// Wear from a race (engine damage and overheating) and the pit droids'
/// repairs after it; returns a line for the results screen.
std::string WearAndRepairRacerParts(RacerProfile& profile, float damage,
                                    bool overheated);

}  // namespace sdl3cpp::services::impl

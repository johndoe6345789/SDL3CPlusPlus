#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_spec.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// The main menu: track, racer, laps, opponents, pod shop, start, quit.
void UpdateRacerMenu(RacerFlow& flow, const RacerNav& nav,
                     const RacerTrackTable& table);

/// The pod shop: buy upgrade levels with truguts.
void UpdateRacerShop(RacerFlow& flow, const RacerNav& nav);

/// Racing, pause and results: pausing, restarting, quitting to the menu,
/// and awarding prize money once the player finishes.
void UpdateRacerRaceFlow(RacerFlow& flow, RacerWorldState& state,
                         const RacerNav& nav, float dt);

/// The upgrade names, in the original's order.
const char* RacerUpgradeName(int index);

/// Truguts for the next level of an upgrade.
int RacerUpgradeCost(int level);

/// Truguts for finishing in `position` (1 = first).
int RacerPrizeFor(int position);

/// The pod's handling with the profile's upgrades applied (5% a level).
RacerPodSpec ApplyRacerUpgrades(RacerPodSpec spec,
                                const RacerProfile& profile);

/// The profile file in the user's preferences folder.
std::filesystem::path RacerProfilePath();
RacerProfile LoadRacerProfile(const std::filesystem::path& path);
bool SaveRacerProfile(const RacerProfile& profile,
                      const std::filesystem::path& path);

}  // namespace sdl3cpp::services::impl

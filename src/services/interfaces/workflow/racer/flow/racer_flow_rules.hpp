#pragma once

#include "services/interfaces/workflow/racer/flow/racer_flow.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_spec.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// The title menu: tournament, free race, Watto's shop, the junkyard,
/// the pit droids, quit.
void UpdateRacerMenu(RacerFlow& flow, const RacerNav& nav,
                     const RacerTrackTable& table);

/// The tournament set-up: circuit, track, racer, winnings split, start.
void UpdateRacerTournament(RacerFlow& flow, const RacerNav& nav,
                           const RacerTrackTable& table);

/// A free race: any open track and racer, laps and rivals; no purse.
void UpdateRacerFreeRace(RacerFlow& flow, const RacerNav& nav,
                         const RacerTrackTable& table);

/// Watto's shop (new parts), the junkyard (used ones) and pit droids.
void UpdateRacerShop(RacerFlow& flow, const RacerNav& nav);
void UpdateRacerJunkyard(RacerFlow& flow, const RacerNav& nav);
void UpdateRacerPitDroids(RacerFlow& flow, const RacerNav& nav);

/// Racing, pause and results: pausing, restarting, quitting to the menu,
/// and booking the race (tournament purse, points, unlocks, wear).
void UpdateRacerRaceFlow(RacerFlow& flow, RacerWorldState& state,
                         const RacerNav& nav, float dt);

/// Whether cutscenes should play here (not in headless or autopilot
/// runs unless RACER_VIDEOS is set; never with RACER_SKIP_VIDEOS).
bool RacerVideosWanted();

/// Plays the cutscenes in order (data/anims names), then goes to `after`.
void PlayRacerVideosThen(RacerFlow& flow, std::vector<std::string> videos,
                         RacerPhase after);

/// Before the first tournament race on a planet: its flyover, once ever.
/// Plays, then returns to the phase the flow is already in.
void QueueRacerPlanetIntro(RacerFlow& flow, const std::string& planet);

/// Books the race once, when the results first show. Only tournament
/// races pay, score and wear the pod's parts, as in the original.
void BookRacerRace(RacerFlow& flow, const RacerWorldState& state);

/// Whether a track (table index) or racer may be chosen yet.
bool RacerTrackOpen(const RacerFlow& flow, const RacerTrackTable& table,
                    int index);
bool RacerRacerOpen(const RacerFlow& flow, int index);

/// Steps `index` by `step` through the open entries of `count`, wrapping.
int RacerStepOpen(int index, int step, int count,
                  const std::vector<bool>& open);

/// Fits a part (new or used), trading in the old one; false (and a
/// notice) when the player cannot afford it.
bool FitRacerPart(RacerFlow& flow, int type, int level, float health,
                  int price);

/// The pod's handling with its fitted parts: 5% a level, scaled by
/// each part's condition.
RacerPodSpec ApplyRacerUpgrades(RacerPodSpec spec,
                                const RacerProfile& profile);

/// The profile file in the user's preferences folder.
std::filesystem::path RacerProfilePath();
RacerProfile LoadRacerProfile(const std::filesystem::path& path);
bool SaveRacerProfile(const RacerProfile& profile,
                      const std::filesystem::path& path);

}  // namespace sdl3cpp::services::impl

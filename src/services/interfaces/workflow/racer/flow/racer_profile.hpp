#pragma once

#include <array>
#include <cstdint>

namespace sdl3cpp::services::impl {

/// The original's seven part types, in its order, each with six parts.
inline constexpr int kRacerUpgradeCount = 7;
inline constexpr int kRacerUpgradeMax = 5;
/// The four tournament circuits and the most tracks one holds.
inline constexpr int kRacerCircuits = 4;
inline constexpr int kRacerCircuitTracks = 7;
inline constexpr int kRacerMaxPitDroids = 4;

/// How a race's purse is shared: across the top four, the top three, or
/// all to the winner (a bigger purse for the bigger risk).
enum class RacerPurseSplit { Fair, Skilled, WinnerTakesAll };

/// What the player keeps between races.
struct RacerProfile {
    int truguts = 400;
    /// The part fitted in each slot (0 = the stock part) and its
    /// condition, 0..1: worn parts give less, broken ones nothing.
    std::array<int, kRacerUpgradeCount> upgrades{};
    std::array<float, kRacerUpgradeCount> health{1, 1, 1, 1, 1, 1, 1};
    int pitDroids = 1;
    int racesRun = 0;         ///< tournament races finished
    int podiums = 0;          ///< top-three finishes: Watto's stock grows
    int circuitsOpen = 1;     ///< Amateur first; each win opens the next
    /// Tracks open in each circuit, and the best place on each (0 none).
    std::array<int, kRacerCircuits> tracksOpen{1, 1, 1, 1};
    std::array<int, kRacerCircuits * kRacerCircuitTracks> best{};
    std::array<int, kRacerCircuits> points{};
    std::uint32_t racers = 0x7F;  ///< bit per racer: the first seven open
    std::uint32_t planetsSeen = 0; ///< bit per planet whose intro played
    bool introSeen = false;        ///< the opening cutscenes, once
};

}  // namespace sdl3cpp::services::impl

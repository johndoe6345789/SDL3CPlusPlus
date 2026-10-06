#pragma once

#include <array>
#include <string>

namespace sdl3cpp::services::impl {

/// Where the game is: the menus, loading a race, racing, paused, or
/// looking at the results.
enum class RacerPhase { Menu, Shop, Loading, Racing, Paused, Results };

/// The original's seven pod upgrades, in its order.
inline constexpr int kRacerUpgradeCount = 7;
inline constexpr int kRacerUpgradeMax = 5;

/// What the player keeps between races: prize money and upgrades.
struct RacerProfile {
    int truguts = 0;
    std::array<int, kRacerUpgradeCount> upgrades{};
};

/// The menus' state and the requests they make of the race steps.
struct RacerFlow {
    RacerPhase phase = RacerPhase::Menu;
    bool initialised = false;
    int menuRow = 0;
    int shopRow = 0;
    int pauseRow = 0;
    int trackIndex = 1;      ///< into the track table (The Boonta Classic)
    int racerIndex = 0;      ///< into the racer table (Anakin Skywalker)
    int laps = 3;
    int opponents = 7;
    bool requestLoad = false;
    bool requestRelease = false;
    bool quit = false;
    int loadingFrames = 0;   ///< frames the loading screen has shown
    float resultsDelay = 0.f;
    int prize = 0;           ///< truguts won in the last race
    bool prizeAwarded = false;
    std::string notice;      ///< a line of feedback (shop, results)
    RacerProfile profile;
};

/// Menu navigation for one frame, as edges (pressed this frame).
struct RacerNav {
    bool up = false, down = false, left = false, right = false;
    bool select = false, back = false;
};

}  // namespace sdl3cpp::services::impl

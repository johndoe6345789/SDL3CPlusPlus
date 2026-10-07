#pragma once

#include "services/interfaces/workflow/racer/flow/racer_profile.hpp"

#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// Where the game is: the title menu, the tournament and free-race
/// set-ups, Watto's shop, the junkyard, the pit droids, a cutscene,
/// loading a race, racing, paused, or looking at the results.
enum class RacerPhase {
    Menu, Shop, Loading, Racing, Paused, Results,
    Tournament, FreeRace, Junkyard, PitDroids, Cutscene
};

/// A used part in the junkyard: cheaper, and worn.
struct RacerJunkOffer {
    int type = 0;
    int level = 0;
    float health = 1.f;
    int price = 0;
};

/// The menus' state and the requests they make of the race steps.
struct RacerFlow {
    RacerPhase phase = RacerPhase::Menu;
    bool initialised = false;
    int menuRow = 0;
    int shopRow = 0;
    int shopLevel = 1;       ///< the part looked at in the shop's row
    int pauseRow = 0;
    int setupRow = 0;        ///< the tournament and free-race screens
    int circuit = 0;
    int circuitTrack = 0;
    RacerPurseSplit split = RacerPurseSplit::Fair;
    bool tournament = false; ///< this race counts for the tournament
    bool unlockAll = false;  ///< RACER_UNLOCK_ALL: every track and racer
    int trackIndex = 0;      ///< into the track table
    int racerIndex = 0;      ///< into the racer table (Anakin Skywalker)
    int laps = 3;
    int opponents = 7;
    bool requestLoad = false;
    bool requestRelease = false;
    bool quit = false;
    int loadingFrames = 0;   ///< frames the loading screen has shown
    float resultsDelay = 0.f;
    int prize = 0;           ///< truguts won in the last race
    int pointsWon = 0;
    bool prizeAwarded = false;
    std::string notice;      ///< a line of feedback (shop, results)
    std::string unlocked;    ///< what the last race opened up
    std::string repairs;     ///< what the pit droids did after it
    RacerProfile profile;
    /// The junkyard's stock this visit (what is sold goes), and the race
    /// count it was drawn for.
    std::vector<RacerJunkOffer> junk;
    int junkDrawnAt = -1;
    /// Cutscenes still to play (data/anims names), then where to go.
    std::vector<std::string> videos;
    RacerPhase afterVideos = RacerPhase::Menu;
    bool skipVideo = false;  ///< select or back pressed during one
};

/// Menu navigation for one frame, as edges (pressed this frame).
struct RacerNav {
    bool up = false, down = false, left = false, right = false;
    bool select = false, back = false;
};

}  // namespace sdl3cpp::services::impl

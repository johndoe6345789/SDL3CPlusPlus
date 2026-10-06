#pragma once

namespace sdl3cpp::services::impl {

/// Lap progress for the player's pod.
struct RacerRaceState {
    int lapsTotal = 3;
    int lap = 1;                 ///< 1-based; lapsTotal + 1 once finished
    int segment = -1;            ///< nearest lap point, -1 before start
    float raceTime = 0.f;
    float lapTime = 0.f;
    float bestLap = 0.f;         ///< 0 until a lap is completed
    float countdown = 3.f;       ///< seconds before the start
    bool finished = false;
};

}  // namespace sdl3cpp::services::impl

#pragma once

namespace sdl3cpp::services::impl {

/// Lap progress for one pod.
struct RacerRaceState {
    int lapsTotal = 3;
    int lap = 1;                 ///< 1-based; lapsTotal + 1 once finished
    int segment = -1;            ///< nearest lap point, -1 before start
    float raceTime = 0.f;
    float lapTime = 0.f;
    float bestLap = 0.f;         ///< 0 until a lap is completed
    float countdown = 3.f;       ///< seconds before the start
    bool finished = false;
    int position = 0;            ///< place in the race, 1 = leading
    int entrants = 1;
};

/// Timers that decide when a pod is lost, kept between frames.
struct RacerRecovery {
    float voidTime = 0.f;
    float stallTime = 0.f;
    int bestPoint = -1;
};

}  // namespace sdl3cpp::services::impl

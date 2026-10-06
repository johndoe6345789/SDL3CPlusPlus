#pragma once

#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"
#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"
#include "services/interfaces/workflow/racer/player/racer_race_state.hpp"
#include "services/interfaces/workflow/racer/world/racer_gpu_types.hpp"

namespace sdl3cpp::services::impl {

/// One computer-flown racer: their own pod model and handling, flown
/// along the lap by the autopilot.
struct RacerOpponent {
    RacerPodInfo racer;
    RacerGpuModel model;
    RacerPodSpec spec;      ///< the stock spec scaled by their pace
    RacerPodState pod;
    RacerRaceState race;
    RacerRecovery recovery;
    float roll = 0.f;
};

}  // namespace sdl3cpp::services::impl

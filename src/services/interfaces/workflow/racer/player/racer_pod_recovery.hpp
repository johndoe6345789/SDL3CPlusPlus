#pragma once

#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

namespace sdl3cpp::services::impl {

/// Why a pod needs putting back on the course, if it does.
enum class RacerRecoveryReason {
    None,
    OffCourse,    ///< over nothing for a second, or falling for long
    Stuck,        ///< pinned against a wall
    NoProgress,   ///< moving but getting nowhere along the lap
};

/// Timers that decide when a pod is lost, kept between frames.
struct RacerRecovery {
    float voidTime = 0.f;
    float stallTime = 0.f;
    int bestPoint = -1;
};

/// Advances the timers by `dt` and says whether (and why) the pod must
/// be put back. Clears the timers when it says so.
RacerRecoveryReason UpdateRacerRecovery(RacerRecovery& recovery,
                                        const RacerWorldState& state,
                                        float throttle, float dt);

/// A short name for logs.
const char* RacerRecoveryName(RacerRecoveryReason reason);

}  // namespace sdl3cpp::services::impl

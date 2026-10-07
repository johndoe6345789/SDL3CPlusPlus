#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"
#include "services/interfaces/workflow/racer/player/racer_race_state.hpp"
#include "services/interfaces/workflow/racer/world/racer_ground.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Why a pod needs putting back on the course, if it does.
enum class RacerRecoveryReason {
    None,
    OffCourse,    ///< over nothing for a second, or falling for long
    Stuck,        ///< pinned against a wall
    NoProgress,   ///< moving but getting nowhere along the lap
};

/// Advances the timers by `dt` and says whether (and why) the pod must
/// be put back. Clears the timers when it says so.
RacerRecoveryReason UpdateRacerRecovery(RacerRecovery& recovery,
                                        const RacerPodState& pod,
                                        const RacerRaceState& race,
                                        const RacerGround& ground,
                                        const std::vector<glm::vec3>& lap,
                                        float throttle, float dt);

/// The lap point to put a lost pod back on: where it was, further on
/// for want of progress, and further each time the same place catches
/// it again. Remembers the choice in `recovery`.
int RacerRespawnPoint(RacerRecovery& recovery, int segment,
                      RacerRecoveryReason reason, int samplesPerKnot);

/// A short name for logs.
const char* RacerRecoveryName(RacerRecoveryReason reason);

}  // namespace sdl3cpp::services::impl

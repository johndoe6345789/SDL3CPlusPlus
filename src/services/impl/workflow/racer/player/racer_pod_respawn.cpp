#include "services/interfaces/workflow/racer/player/racer_pod_recovery.hpp"

#include <algorithm>
#include <cstdlib>

namespace sdl3cpp::services::impl {

const char* RacerRecoveryName(RacerRecoveryReason reason) {
    switch (reason) {
    case RacerRecoveryReason::OffCourse: return "off course";
    case RacerRecoveryReason::Stuck: return "stuck";
    case RacerRecoveryReason::NoProgress: return "no progress";
    case RacerRecoveryReason::None: break;
    }
    return "none";
}

int RacerRespawnPoint(RacerRecovery& recovery, int segment,
                      RacerRecoveryReason reason, int samplesPerKnot) {
    segment = std::max(0, segment);
    // Put back where it was put back before: something there keeps
    // catching it, so each time go a little further on.
    const bool again = recovery.lastRespawn >= 0 &&
                       std::abs(segment - recovery.lastRespawn) <=
                           2 * samplesPerKnot;
    recovery.streak = again ? recovery.streak + 1 : 0;
    const int ahead =
        (reason == RacerRecoveryReason::NoProgress ? 2 : 0) +
        2 * recovery.streak;
    recovery.lastRespawn = segment + ahead * samplesPerKnot;
    return recovery.lastRespawn;
}

}  // namespace sdl3cpp::services::impl

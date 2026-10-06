#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// Adjusts an AI pod's controls for the pods around it: steers to pass
/// a pod in its line ahead on the side with more room, and lifts off
/// (no boost) rather than ram a slower pod close in front.
void AvoidRacerTraffic(const RacerPodState& pod,
                       const std::vector<const RacerPodState*>& others,
                       RacerPodInput& input);

}  // namespace sdl3cpp::services::impl

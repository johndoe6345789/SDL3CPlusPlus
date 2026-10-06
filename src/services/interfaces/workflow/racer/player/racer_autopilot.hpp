#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Steers a pod along the lap: aims a few points ahead of the nearest
/// one, lifts off for sharp turns, and boosts on straights while the
/// engines are cool. Drives the headless race checks, and is the
/// starting point for AI opponents.
RacerPodInput RacerAutopilot(const RacerPodState& pod,
                             const std::vector<glm::vec3>& lapPoints,
                             int nearestPoint);

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// Steers a pod along the lap: aims a little ahead of the nearest
/// point, and plans its speed from the turns coming up and what this
/// pod can manage (its turn rate, how much it keeps at speed, its air
/// brake), boosting where the line is clear and the engines cool.
/// Drives the headless race checks and every AI rival.
RacerPodInput RacerAutopilot(const RacerPodState& pod,
                             const std::vector<glm::vec3>& lapPoints,
                             int nearestPoint, const RacerPodSpec& spec);

/// The fastest a pod with `spec` can hold round a turn of `radius`
/// metres: where its speed-dependent turn rate times the radius keeps
/// up with the speed. Pure, so it is testable.
float RacerCornerSpeed(const RacerPodSpec& spec, float radius);

/// The speed to carry now so the pod can still brake to every turn's
/// corner speed on the lap ahead (up to `horizon` metres on).
float RacerPlannedSpeed(const std::vector<glm::vec3>& lapPoints,
                        int nearestPoint, const RacerPodSpec& spec,
                        float horizon);

}  // namespace sdl3cpp::services::impl

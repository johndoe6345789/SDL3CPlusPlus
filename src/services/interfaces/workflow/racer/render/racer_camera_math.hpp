#pragma once

#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

namespace sdl3cpp::services::impl {

/// Shortest signed turn from heading `from` to `to`, in -pi..pi, so an
/// eased heading never unwinds the long way round.
float RacerAngleDelta(float from, float to);

/// The lowest a camera eye may sit: 1.5 m above any ground on the way
/// from the pod back to it, so hills never come between the camera and
/// the pod, nor swallow the camera.
float RacerCameraClearance(const RacerWorldState& state,
                           const glm::vec3& pod, const glm::vec3& eye);

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Shortest signed turn from heading `from` to `to`, in -pi..pi, so an
/// eased heading never unwinds the long way round.
float RacerAngleDelta(float from, float to);

/// A matrix as a 16-number JSON array (column-major), as camera.state
/// expects it.
nlohmann::json RacerMatrixJson(const glm::mat4& matrix);

/// The lowest a camera eye may sit so its sight line to the pod clears
/// the ground by 1.5 m all the way: hills never come between the camera
/// and the pod, nor swallow the camera.
float RacerCameraClearance(const RacerWorldState& state,
                           const glm::vec3& pod, const glm::vec3& eye);

/// The eye moved in toward `focus` to just short of the first wall
/// between them, so canyon walls rarely hide the pod; `eye` when clear.
/// Never closer than `nearest`, which keeps it out of the pod itself.
glm::vec3 RacerCameraPullIn(const RacerWorldState& state,
                            const glm::vec3& focus, const glm::vec3& eye,
                            float nearest);

}  // namespace sdl3cpp::services::impl

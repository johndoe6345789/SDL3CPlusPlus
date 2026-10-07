#pragma once

#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <glm/glm.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

/// The game's AI pods ride the track spline with only a small sideways
/// offset. This keeps an AI-driven pod within `maxOffset` metres of the
/// racing line across the ground, drawing it back at up to `pullSpeed`
/// metres a second, so narrow ledges and hairpins do not throw it off.
/// The player's own pod is never guided.
/// How far AI pods may stray from the line, and how fast they are drawn
/// back: room to pass one another, not to leave a ten-metre ledge.
inline constexpr float kRacerAiLineOffset = 3.5f;
inline constexpr float kRacerAiLinePull = 20.f;

void GuideRacerPodToLine(RacerPodState& pod,
                         const std::vector<glm::vec3>& lapPoints,
                         int nearestPoint, float maxOffset, float pullSpeed,
                         float dt);

}  // namespace sdl3cpp::services::impl

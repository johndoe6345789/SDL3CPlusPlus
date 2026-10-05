#pragma once

#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

namespace sdl3cpp::services::impl {

/// The lap point nearest `position`. With `previous` >= 0 only points
/// near it are searched, so a pod on one section never snaps to a
/// parallel section of the course that happens to pass close by.
int NearestRacerLapPoint(const std::vector<glm::vec3>& points,
                         const glm::vec3& position, int previous);

/// Advances the race clock and counts laps: a lap completes when the
/// nearest point wraps from the last quarter of the lap to the first.
void AdvanceRacerRace(RacerRaceState& race, int point, int pointCount,
                      float dt);

}  // namespace sdl3cpp::services::impl

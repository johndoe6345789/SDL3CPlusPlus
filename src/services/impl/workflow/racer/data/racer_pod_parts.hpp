#pragma once

#include "racer_model_context.hpp"

namespace sdl3cpp::services::impl::racer_model_detail {

// Parts sit under a 0.02 scale the game replaces when it places them.
// An eighth of the raw size matches the game's own handling table (a
// 5-10 unit contact radius, a 5 unit hover over the engines) and gives
// Anakin's engines about 7 m, as in the film.
inline constexpr float kPodPartScale = 0.125f;

/// A part's box, already at pod scale.
struct PodPartBounds {
    glm::vec3 lo{1e9f};
    glm::vec3 hi{-1e9f};
    glm::vec3 Size() const { return hi - lo; }
    glm::vec3 Centre() const { return 0.5f * (lo + hi); }
};

PodPartBounds BoundsOf(const RacerModel& part);

/// Scales a part to pod size, moves it by `by` and adds it to `into`.
void MoveAndMerge(RacerModel& part, const glm::vec3& by, RacerModel& into);

}  // namespace sdl3cpp::services::impl::racer_model_detail

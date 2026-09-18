#pragma once

#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Whether engine (x, z) has travelled far enough from the origin that
/// engine space should be re-centred: past `radius` metres, the
/// origin's frozen ground scale is off by more than it should be
/// (0.5% at 40 km north of London) and float precision starts to thin.
bool Fs2024RebaseDue(float x, float z, float radius);

/// Re-centres `origin` on the level-14 tile under engine (x, z) -- the
/// same quadtree, a new anchor and a new ground scale -- and returns
/// that same point on the Earth in the new engine space. Every resident
/// tile is in the old space, so the caller drops them all.
glm::vec2 RebaseFs2024Origin(Fs2024GeoOrigin& origin, float x, float z);

}  // namespace sdl3cpp::services::impl

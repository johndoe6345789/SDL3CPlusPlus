#pragma once

#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// Whether engine (x, z) has travelled far enough from the origin that
/// engine space should be re-centred: past `radius` metres float
/// precision starts to thin.
bool Fs2024RebaseDue(float x, float z, float radius);

/// How a re-base moved engine space.
struct Fs2024Rebase {
    /// Subtract from every engine x, z: the same spot on the Earth in
    /// the new space.
    glm::vec2 shift{0.f};
    /// Finest-level tiles the origin moved by (multiples of eight, so
    /// every streamed level moves by whole tiles).
    int tilesX = 0, tilesY = 0;
    /// The ground scale changed too: nothing built in the old space
    /// stands true in the new one.
    bool rescaled = false;
};

/// Moves `origin` onto the coarsest tile under engine (x, z). Engine
/// metres keep their scale -- a pure shift by whole tiles, so every
/// resident tile stays valid where it is, only moved -- until the
/// scale that latitude would have drifts more than 1% from the one in
/// use (about a degree of latitude north or south), when the origin is
/// made afresh.
Fs2024Rebase RebaseFs2024Origin(Fs2024GeoOrigin& origin, float x, float z);

}  // namespace sdl3cpp::services::impl

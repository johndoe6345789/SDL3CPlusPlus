#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"
#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"

namespace sdl3cpp::services::impl {

/// A square of `cells` x `cells` grid cells starting at sample
/// (firstI, firstJ). Its texture repeats every `uvMetres` across the world.
struct SwitchbackChunkSpec {
    int firstI = 0;
    int firstJ = 0;
    int cells = 128;
    float stepM = 1.f;
    float uvMetres = 20.f;
};

/// Heights come from the map. x and z are centred on the origin, the same
/// layout the collision heightfield uses.
GeometryPlaneMesh BuildSwitchbackChunkMesh(const SwitchbackHeightmap& map,
                                           const SwitchbackChunkSpec& spec);

}  // namespace sdl3cpp::services::impl

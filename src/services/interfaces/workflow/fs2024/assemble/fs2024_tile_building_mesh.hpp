#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_buildings.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// A tile's buildings meshed in its local space: walls and roofs apart,
/// so each draws with its own texture.
struct Fs2024BuildingMeshCpu {
    std::vector<BspRenderVertex> wallVertices, roofVertices;
    std::vector<std::uint32_t> wallIndices, roofIndices;
};

/// Raises every plan on `field` (the same tile's local heightfield):
/// each building stands on the lowest ground under its own footprint,
/// so on a slope it is sunk into the uphill side rather than floating
/// off the downhill one -- how a real terrace meets a hill.
Fs2024BuildingMeshCpu MeshFs2024TileBuildings(
    const std::vector<Fs2024BuildingPlan>& plans,
    const Fs2024Heightfield& field);

}  // namespace sdl3cpp::services::impl

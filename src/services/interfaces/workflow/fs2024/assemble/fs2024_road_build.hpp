#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_water_build.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// How wide a road of FS2024's class bit is, in metres; 0 for the
/// classes not drawn (the footways and service paths of bit 30). The
/// bit is an importance ordinal, lower grander.
float Fs2024RoadWidth(int classBit);

/// Where a bridge's deck stands at `p`: above `ground`, and clear of
/// any `water` there (at its `levels`) by five metres a storey of its
/// own `level`.
float Fs2024BridgeDeck(const Point2& p, float ground, int level,
                       const std::vector<Fs2024VecShape>& water,
                       const std::vector<float>& levels);

/// Ribbons along every road, draped on `field` (tile space, already
/// carved for water), texture v running along the road in metres/8.
/// Tunnels are left out. A bridge's deck is held clear of the water it
/// crosses -- `water` and its `levels` from BuildFs2024Water -- five
/// metres a storey of its own level.
Fs2024TerrainChunkMesh BuildFs2024Roads(
    const std::vector<Fs2024VecShape>& roads,
    const std::vector<Fs2024VecShape>& water,
    const std::vector<float>& levels, const Fs2024Heightfield& field);

}  // namespace sdl3cpp::services::impl

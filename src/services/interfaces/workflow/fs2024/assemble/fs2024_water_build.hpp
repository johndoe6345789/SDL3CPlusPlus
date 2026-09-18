#pragma once

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vec_shapes.hpp"
#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

/// A tile's water, ready to draw, and the level each outline lies at.
struct Fs2024WaterBuild {
    Fs2024TerrainChunkMesh mesh;
    std::vector<float> levels;  ///< per `water` outline
};

/// Lays each water outline flat at the lowest ground inside it and
/// lowers the ground under it a few metres below that, so the surface
/// shows everywhere inside and the banks rise out of it: FS2024's DEM
/// (~95 m a sample) knows nothing of a river's channel. `field` is the
/// tile's own, still in tile space; carve it before meshing the ground.
Fs2024WaterBuild BuildFs2024Water(const std::vector<Fs2024VecShape>& water,
                                  Fs2024Heightfield& field);

/// Whether `p` lies inside `ring` (even-odd).
bool Fs2024RingContains(const std::vector<Point2>& ring, const Point2& p);

}  // namespace sdl3cpp::services::impl

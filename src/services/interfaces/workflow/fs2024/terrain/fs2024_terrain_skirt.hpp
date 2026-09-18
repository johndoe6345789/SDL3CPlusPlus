#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"

namespace sdl3cpp::services::impl {

/// Hangs a wall `depth` metres deep from every edge of a tile's ground
/// -- `mesh` being one whole `columns` x `rows` grid, row-major -- facing
/// outwards. Where a tile meets a neighbour of another detail level
/// their edges sample the ground at different points, and the skirt
/// fills the sliver of sky that would otherwise show between them.
void AppendFs2024TerrainSkirt(Fs2024TerrainChunkMesh& mesh, int columns,
                              int rows, float depth);

}  // namespace sdl3cpp::services::impl

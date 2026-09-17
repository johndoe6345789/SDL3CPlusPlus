#pragma once

#include "services/interfaces/workflow/fs2024/terrain/fs2024_heightfield.hpp"
#include "services/interfaces/workflow/rendering/bsp_types.hpp"

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One square block of the ground, ready to upload.
struct Fs2024TerrainChunkMesh {
    std::vector<BspRenderVertex> vertices;
    std::vector<std::uint32_t> indices;
    glm::vec3 min{0.f};
    glm::vec3 max{0.f};
};

/// Up-facing unit normal at a grid vertex, from its neighbours.
glm::vec3 Fs2024TerrainNormal(const Fs2024Heightfield& field, int column,
                              int row);

/// The block of `cells` x `cells` grid cells whose north-west vertex is
/// (column, row), clipped to the grid. uv spans the whole field once, so
/// the baked ground map lines up with every block.
Fs2024TerrainChunkMesh BuildFs2024TerrainChunk(
    const Fs2024Heightfield& field, int column, int row, int cells);

}  // namespace sdl3cpp::services::impl

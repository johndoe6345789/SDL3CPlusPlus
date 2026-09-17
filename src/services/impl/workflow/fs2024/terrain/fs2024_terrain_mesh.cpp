#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

glm::vec3 Fs2024TerrainNormal(const Fs2024Heightfield& field, int column,
                              int row) {
    const int west = std::max(column - 1, 0);
    const int east = std::min(column + 1, field.columns - 1);
    const int north = std::max(row - 1, 0);
    const int south = std::min(row + 1, field.rows - 1);
    const float dx = (field.At(east, row) - field.At(west, row)) /
                     (static_cast<float>(east - west) * field.spacing);
    const float dz = (field.At(column, south) - field.At(column, north)) /
                     (static_cast<float>(south - north) * field.spacing);
    return glm::normalize(glm::vec3(-dx, 1.f, -dz));
}

Fs2024TerrainChunkMesh BuildFs2024TerrainChunk(
    const Fs2024Heightfield& field, int column, int row, int cells) {
    const int lastColumn = std::min(column + cells, field.columns - 1);
    const int lastRow = std::min(row + cells, field.rows - 1);
    const int width = lastColumn - column + 1;
    const float uScale = 1.f / static_cast<float>(field.columns - 1);
    const float vScale = 1.f / static_cast<float>(field.rows - 1);

    Fs2024TerrainChunkMesh mesh;
    mesh.min = glm::vec3(1e30f);
    mesh.max = glm::vec3(-1e30f);
    for (int r = row; r <= lastRow; ++r) {
        for (int c = column; c <= lastColumn; ++c) {
            const glm::vec3 p = field.Position(c, r);
            const glm::vec3 n = Fs2024TerrainNormal(field, c, r);
            mesh.vertices.push_back(BspRenderVertex{
                p.x, p.y, p.z, static_cast<float>(c) * uScale,
                static_cast<float>(r) * vScale, 0.f, 0.f, n.x, n.y, n.z});
            mesh.min = glm::min(mesh.min, p);
            mesh.max = glm::max(mesh.max, p);
        }
    }

    // Counter-clockwise seen from above, split along the same diagonal
    // as the collision shape: (c+1, r) to (c, r+1).
    for (int r = 0; r + row < lastRow; ++r) {
        for (int c = 0; c + column < lastColumn; ++c) {
            const auto nw = static_cast<std::uint32_t>(r * width + c);
            const auto ne = nw + 1u;
            const auto sw = nw + static_cast<std::uint32_t>(width);
            const auto se = sw + 1u;
            mesh.indices.insert(mesh.indices.end(),
                                {nw, sw, ne, ne, sw, se});
        }
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl

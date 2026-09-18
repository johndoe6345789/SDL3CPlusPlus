#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_skirt.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// The grid's border, clockwise seen from above (x east, z south): the
/// north edge west to east, then down the east edge, back along the
/// south and up the west, closing where it began.
std::vector<std::uint32_t> Border(int columns, int rows) {
    std::vector<std::uint32_t> ring;
    const auto at = [&](int c, int r) {
        return static_cast<std::uint32_t>(r * columns + c);
    };
    for (int c = 0; c < columns - 1; ++c) ring.push_back(at(c, 0));
    for (int r = 0; r < rows - 1; ++r) ring.push_back(at(columns - 1, r));
    for (int c = columns - 1; c > 0; --c) ring.push_back(at(c, rows - 1));
    for (int r = rows - 1; r > 0; --r) ring.push_back(at(0, r));
    ring.push_back(ring.front());
    return ring;
}

}  // namespace

void AppendFs2024TerrainSkirt(Fs2024TerrainChunkMesh& mesh, int columns,
                              int rows, float depth) {
    if (columns < 2 || rows < 2) return;
    const std::vector<std::uint32_t> ring = Border(columns, rows);
    const auto first = static_cast<std::uint32_t>(mesh.vertices.size());
    for (const std::uint32_t top : ring) {
        BspRenderVertex bottom = mesh.vertices[top];
        bottom.y -= depth;
        mesh.vertices.push_back(bottom);
    }
    for (std::uint32_t i = 0; i + 1 < ring.size(); ++i) {
        const std::uint32_t top = ring[i], nextTop = ring[i + 1];
        const std::uint32_t bottom = first + i, nextBottom = bottom + 1;
        // Counter-clockwise seen from outside the tile.
        mesh.indices.insert(mesh.indices.end(), {top, nextTop, bottom,
                                                 nextTop, nextBottom,
                                                 bottom});
    }
    mesh.min.y -= depth;
}

}  // namespace sdl3cpp::services::impl

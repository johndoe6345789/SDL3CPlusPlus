#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_mesh.hpp"

#include <cstddef>
#include <cstdint>

namespace sdl3cpp::services::impl {
namespace {

std::uint16_t VertexIndex(int side, int j, int i) {
    return static_cast<std::uint16_t>(j * side + i);
}

}  // namespace

GeometryPlaneMesh BuildSwitchbackChunkMesh(const SwitchbackHeightmap& map,
                                           const SwitchbackChunkSpec& spec) {
    GeometryPlaneMesh mesh;
    const int side = spec.cells + 1;
    const float half = static_cast<float>(map.size - 1) * spec.stepM * 0.5f;
    mesh.vertices.reserve(static_cast<std::size_t>(side) * side);
    for (int j = 0; j < side; ++j) {
        const int gridJ = spec.firstJ + j;
        const float z = -half + static_cast<float>(gridJ) * spec.stepM;
        const std::size_t row =
            static_cast<std::size_t>(gridJ) * static_cast<std::size_t>(map.size);
        for (int i = 0; i < side; ++i) {
            const int gridI = spec.firstI + i;
            const float x = -half + static_cast<float>(gridI) * spec.stepM;
            const float y = map.metres[row + static_cast<std::size_t>(gridI)];
            mesh.vertices.push_back(
                {x, y, z, x / spec.uvMetres, z / spec.uvMetres});
        }
    }
    mesh.indices.reserve(static_cast<std::size_t>(spec.cells) * spec.cells * 6);
    for (int j = 0; j < spec.cells; ++j) {
        for (int i = 0; i < spec.cells; ++i) {
            const std::uint16_t a = VertexIndex(side, j, i);
            const std::uint16_t b = VertexIndex(side, j, i + 1);
            const std::uint16_t c = VertexIndex(side, j + 1, i);
            const std::uint16_t d = VertexIndex(side, j + 1, i + 1);
            mesh.indices.insert(mesh.indices.end(), {a, c, b, b, c, d});
        }
    }
    return mesh;
}

}  // namespace sdl3cpp::services::impl

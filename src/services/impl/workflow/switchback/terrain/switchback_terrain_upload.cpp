#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_upload.hpp"

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl {

void UploadSwitchbackChunks(SDL_GPUDevice* device,
                            const SwitchbackHeightmap& map,
                            const SwitchbackChunkSpec& base,
                            SwitchbackTerrainState& state) {
    const int perSide = (map.size - 1) / base.cells;
    state.chunks.reserve(static_cast<std::size_t>(perSide) * perSide);
    for (int j = 0; j < perSide; ++j) {
        for (int i = 0; i < perSide; ++i) {
            SwitchbackChunkSpec spec = base;
            spec.firstI = i * base.cells;
            spec.firstJ = j * base.cells;
            const GeometryPlaneMesh mesh = BuildSwitchbackChunkMesh(map, spec);
            const GeometryPlaneBuffers buffers =
                UploadGeometryPlaneMesh(device, mesh);
            SwitchbackTerrainChunk chunk;
            chunk.vertexBuffer = buffers.vertexBuffer;
            chunk.indexBuffer = buffers.indexBuffer;
            chunk.indexCount = static_cast<std::uint32_t>(mesh.indices.size());
            state.chunks.push_back(chunk);
        }
    }
}

void ReleaseSwitchbackChunks(SDL_GPUDevice* device,
                             SwitchbackTerrainState& state) {
    for (const SwitchbackTerrainChunk& chunk : state.chunks) {
        SDL_ReleaseGPUBuffer(device, chunk.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, chunk.indexBuffer);
    }
    state.chunks.clear();
}

std::string InstallSwitchbackTerrain(SDL_GPUDevice* device,
                                     btDiscreteDynamicsWorld& world,
                                     const SwitchbackTrackLayout& layout,
                                     int cells, float uvMetres,
                                     SwitchbackTerrainState& state) {
    if (cells <= 0 || (layout.heightmap.size - 1) % cells != 0) {
        return "chunk_cells " + std::to_string(cells) + " does not divide " +
               std::to_string(layout.heightmap.size - 1);
    }
    SwitchbackChunkSpec base;
    base.cells = cells;
    base.stepM = layout.stepM;
    base.uvMetres = uvMetres;
    try {
        UploadSwitchbackChunks(device, layout.heightmap, base, state);
    } catch (const std::runtime_error& error) {
        ReleaseSwitchbackChunks(device, state);
        return error.what();
    }
    BuildSwitchbackCollision(world, layout.heightmap, layout.stepM,
                             layout.heightMaxM, state);
    return {};
}

}  // namespace sdl3cpp::services::impl

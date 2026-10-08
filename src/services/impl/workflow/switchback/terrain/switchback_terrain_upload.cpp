#include "services/interfaces/workflow/switchback/terrain/switchback_terrain_upload.hpp"

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

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

}  // namespace sdl3cpp::services::impl

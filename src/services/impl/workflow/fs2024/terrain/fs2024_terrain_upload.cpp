#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_upload.hpp"

#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_mesh.hpp"
#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

namespace sdl3cpp::services::impl {

std::size_t UploadFs2024TerrainChunks(SDL_GPUDevice* device,
                                      Fs2024TerrainState& state,
                                      int cells) {
    const Fs2024Heightfield& field = state.field;
    std::size_t triangles = 0;
    for (int row = 0; row < field.rows - 1; row += cells) {
        for (int column = 0; column < field.columns - 1; column += cells) {
            const Fs2024TerrainChunkMesh mesh =
                BuildFs2024TerrainChunk(field, column, row, cells);
            if (mesh.indices.empty()) continue;
            const BspGeometryBuffers buffers =
                UploadBspGeometryBuffers(device, mesh.vertices,
                                         mesh.indices);
            Fs2024TerrainChunkGpu chunk;
            chunk.vertexBuffer = buffers.vertex_buffer;
            chunk.indexBuffer = buffers.index_buffer;
            chunk.indexCount =
                static_cast<std::uint32_t>(mesh.indices.size());
            chunk.min = mesh.min;
            chunk.max = mesh.max;
            state.chunks.push_back(chunk);
            triangles += mesh.indices.size() / 3u;
        }
    }
    return triangles;
}

void ReleaseFs2024TerrainChunks(SDL_GPUDevice* device,
                                Fs2024TerrainState& state) {
    if (device) {
        for (const Fs2024TerrainChunkGpu& chunk : state.chunks) {
            SDL_ReleaseGPUBuffer(device, chunk.vertexBuffer);
            SDL_ReleaseGPUBuffer(device, chunk.indexBuffer);
        }
    }
    state.chunks.clear();
}

}  // namespace sdl3cpp::services::impl

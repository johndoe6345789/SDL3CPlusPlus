#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_chunk.hpp"

namespace sdl3cpp::services::impl {

void DrawFs2024Chunk(SDL_GPURenderPass* pass,
                     const Fs2024TerrainChunkGpu& chunk,
                     const Fs2024Frustum& frustum, const glm::vec3& offset) {
    if (chunk.indexCount == 0) return;
    if (!Fs2024BoxVisible(frustum, chunk.min + offset, chunk.max + offset)) {
        return;
    }
    SDL_GPUBufferBinding vb{};
    vb.buffer = chunk.vertexBuffer;
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib{};
    ib.buffer = chunk.indexBuffer;
    SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_DrawGPUIndexedPrimitives(pass, chunk.indexCount, 1, 0, 0, 0);
}

}  // namespace sdl3cpp::services::impl

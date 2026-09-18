#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_tile.hpp"

#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// Chunks are tile-local, so their bounds move with the tile.
void DrawChunk(SDL_GPURenderPass* pass, const Fs2024TerrainChunkGpu& chunk,
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

void PushOffset(SDL_GPUCommandBuffer* cmd, Fs2024TerrainVertexUniforms vertex,
                const glm::vec3& offset) {
    vertex.originOffset = glm::vec4(offset, 0.f);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
}

}  // namespace

void DrawFs2024TileGround(SDL_GPURenderPass* pass, SDL_GPUCommandBuffer* cmd,
                          const Fs2024LoadedTile& tile,
                          const Fs2024World& world,
                          Fs2024TerrainVertexUniforms vertex,
                          const Fs2024Frustum& frustum) {
    if (!tile.classMap || !world.materialArray) return;
    PushOffset(cmd, vertex, tile.offset);
    const SDL_GPUTextureSamplerBinding samplers[2] = {
        {tile.classMap, tile.classSampler},
        {world.materialArray, world.materialSampler}};
    SDL_BindGPUFragmentSamplers(pass, 0, samplers, 2);
    for (const Fs2024TerrainChunkGpu& chunk : tile.terrain.chunks) {
        DrawChunk(pass, chunk, frustum, tile.offset);
    }
}

void DrawFs2024TileBuildings(SDL_GPURenderPass* pass,
                             SDL_GPUCommandBuffer* cmd,
                             const Fs2024LoadedTile& tile,
                             Fs2024TerrainVertexUniforms vertex,
                             const Fs2024TerrainFragmentUniforms& fragment,
                             const Fs2024Frustum& frustum,
                             SDL_GPUTextureSamplerBinding wall,
                             SDL_GPUTextureSamplerBinding roof) {
    if (tile.buildingChunk.indexCount == 0) return;
    PushOffset(cmd, vertex, tile.offset);
    SDL_PushGPUFragmentUniformData(cmd, 0, &fragment, sizeof(fragment));
    if (wall.texture) {
        SDL_BindGPUFragmentSamplers(pass, 0, &wall, 1);
        DrawChunk(pass, tile.buildingChunk, frustum, tile.offset);
    }
    if (roof.texture) {
        SDL_BindGPUFragmentSamplers(pass, 0, &roof, 1);
        DrawChunk(pass, tile.buildingRoofChunk, frustum, tile.offset);
    }
}

}  // namespace sdl3cpp::services::impl

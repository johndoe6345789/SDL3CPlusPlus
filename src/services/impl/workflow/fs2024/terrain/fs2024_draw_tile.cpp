#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_tile.hpp"

#include "services/interfaces/workflow/fs2024/terrain/fs2024_draw_chunk.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_world.hpp"

namespace sdl3cpp::services::impl {
namespace {

void PushOffset(SDL_GPUCommandBuffer* cmd, Fs2024TerrainVertexUniforms vertex,
                const glm::vec3& offset) {
    vertex.originOffset = glm::vec4(offset, 0.f);
    SDL_PushGPUVertexUniformData(cmd, 0, &vertex, sizeof(vertex));
}

}  // namespace

SDL_GPUTextureSamplerBinding Fs2024ContextTexture(
    const WorkflowContext& context, const std::string& key) {
    return {context.Get<SDL_GPUTexture*>(key + "_gpu", nullptr),
            context.Get<SDL_GPUSampler*>(key + "_sampler", nullptr)};
}

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
        DrawFs2024Chunk(pass, chunk, frustum, tile.offset);
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
        DrawFs2024Chunk(pass, tile.buildingChunk, frustum, tile.offset);
    }
    if (roof.texture) {
        SDL_BindGPUFragmentSamplers(pass, 0, &roof, 1);
        DrawFs2024Chunk(pass, tile.buildingRoofChunk, frustum, tile.offset);
    }
}

}  // namespace sdl3cpp::services::impl

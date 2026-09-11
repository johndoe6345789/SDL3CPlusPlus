#include "services/interfaces/workflow/gta5/gta5_draw_bind.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

void BindGta5BatchShared(const Gta5DrawContext& draw,
                         const Gta5InstanceBatch& batch) {
    SDL_BindGPUVertexStorageBuffers(draw.pass, 0, &batch.buffer, 1);
    // Once a frame: each group's offset travels as first_instance.
    Gta5InstancedUniforms vu = {};
    const glm::mat4 viewProj = draw.proj * draw.view;
    std::memcpy(vu.viewProj, glm::value_ptr(viewProj), sizeof(vu.viewProj));
    std::memcpy(vu.shadowVP, glm::value_ptr(draw.shadowVP),
                sizeof(vu.shadowVP));
    vu.cameraPos[0] = draw.cameraPos.x;
    vu.cameraPos[1] = draw.cameraPos.y;
    vu.cameraPos[2] = draw.cameraPos.z;
    SDL_PushGPUVertexUniformData(draw.cmd, 0, &vu, sizeof(vu));
}

int BindGta5SubMeshTextures(const Gta5DrawContext& draw,
                            const Gta5SubMesh& sub, SDL_GPUTexture*& bound) {
    SDL_GPUTexture* texture = sub.texture ? sub.texture : draw.texture;
    SDL_GPUSampler* sampler = sub.texture ? sub.sampler : draw.sampler;
    // The shaders read the sun's shadow map after their own textures.
    if (!texture || !sampler || !draw.shadowTexture || !draw.shadowSampler) {
        return -1;
    }
    const SDL_GPUTextureSamplerBinding shadow = {draw.shadowTexture,
                                                 draw.shadowSampler};
    if (sub.terrain) {
        SDL_GPUTextureSamplerBinding layers[6];  // 4 layers, mask, shadow
        layers[5] = shadow;
        for (int i = 0; i < 5; ++i) {
            layers[i] = sub.layers[i]
                            ? SDL_GPUTextureSamplerBinding{sub.layers[i],
                                                           sub.layerSamplers[i]}
                            : SDL_GPUTextureSamplerBinding{texture, sampler};
        }
        SDL_BindGPUFragmentSamplers(draw.pass, 0, layers, 6);
        bound = nullptr;
        return 1;
    }
    if (texture == bound) return 0;
    const SDL_GPUTextureSamplerBinding bindings[2] = {{texture, sampler},
                                                     shadow};
    SDL_BindGPUFragmentSamplers(draw.pass, 0, bindings, 2);
    bound = texture;
    return 1;
}

void BindGta5ArenaBlock(const Gta5StreamState& state,
                        const Gta5DrawContext& draw, int block) {
    SDL_GPUBufferBinding vb = {state.arena.Vertices(block), 0};
    SDL_BindGPUVertexBuffers(draw.pass, 0, &vb, 1);
    SDL_GPUBufferBinding ib = {state.arena.Indices(block), 0};
    SDL_BindGPUIndexBuffer(draw.pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
}

}  // namespace sdl3cpp::services::impl

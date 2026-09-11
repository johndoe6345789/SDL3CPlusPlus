#include "services/interfaces/workflow/gta5/gta5_shadow.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

int DrawGta5ShadowCasters(const Gta5StreamState& state,
                          const Gta5InstanceBatch& batch,
                          SDL_GPUCommandBuffer* cmd, SDL_GPURenderPass* pass,
                          const glm::mat4& lightViewProj,
                          SDL_GPUTexture* blank, SDL_GPUSampler* sampler) {
    if (!batch.buffer || batch.items.empty()) return 0;
    SDL_BindGPUVertexStorageBuffers(pass, 0, &batch.buffer, 1);
    Gta5InstancedUniforms vu = {};
    std::memcpy(vu.viewProj, glm::value_ptr(lightViewProj),
                sizeof(vu.viewProj));
    SDL_PushGPUVertexUniformData(cmd, 0, &vu, sizeof(vu));
    SDL_GPUTexture* bound = nullptr;
    int block = -1, drawn = 0;
    float pushed = -1.f;
    for (const Gta5DrawItem& item : batch.items) {
        const Gta5SubMesh& sub = *item.sub;
        if (sub.slot.block < 0 || sub.blend) continue;
        // Only cutouts read their texture; terrain's alpha means a mask.
        const float cutoff = sub.DrawKind() == 1 ? sub.surface[3] : 0.f;
        const bool own = cutoff > 0.f && sub.texture;
        SDL_GPUTexture* texture = own ? sub.texture : blank;
        if (!texture) continue;
        if (texture != bound) {
            const SDL_GPUTextureSamplerBinding binding = {
                texture, own ? sub.sampler : sampler};
            SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);
            bound = texture;
        }
        if (cutoff != pushed) {
            const float surface[4] = {1.f, 1.f, 1.f, cutoff};
            SDL_PushGPUFragmentUniformData(cmd, 0, surface, sizeof(surface));
            pushed = cutoff;
        }
        if (sub.slot.block != block) {
            block = sub.slot.block;
            const SDL_GPUBufferBinding vb = {state.arena.Vertices(block), 0};
            SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
            const SDL_GPUBufferBinding ib = {state.arena.Indices(block), 0};
            SDL_BindGPUIndexBuffer(pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        }
        SDL_DrawGPUIndexedPrimitives(
            pass, sub.indexCount, item.count, sub.slot.firstIndex,
            static_cast<Sint32>(sub.slot.vertexOffset), item.first);
        ++drawn;
    }
    return drawn;
}

}  // namespace sdl3cpp::services::impl

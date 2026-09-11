#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

Gta5InstancedUniforms MakeUniforms(const Gta5DrawContext& draw) {
    Gta5InstancedUniforms vu = {};
    const glm::mat4 viewProj = draw.proj * draw.view;
    std::memcpy(vu.viewProj, glm::value_ptr(viewProj), sizeof(vu.viewProj));
    std::memcpy(vu.shadowVP, glm::value_ptr(draw.shadowVP),
                sizeof(vu.shadowVP));
    vu.cameraPos[0] = draw.cameraPos.x;
    vu.cameraPos[1] = draw.cameraPos.y;
    vu.cameraPos[2] = draw.cameraPos.z;
    return vu;
}

}  // namespace

int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw, int* textureBinds) {
    const Gta5InstanceBatch& batch = state.batch;
    if (!draw.pass || !draw.cmd || !batch.buffer || batch.items.empty()) {
        return 0;
    }
    SDL_BindGPUVertexStorageBuffers(draw.pass, 0, &batch.buffer, 1);
    // Once a frame: the group offset travels as first_instance.
    const Gta5InstancedUniforms vu = MakeUniforms(draw);
    SDL_PushGPUVertexUniformData(draw.cmd, 0, &vu, sizeof(vu));
    rendering::FragmentUniformData fu = draw.fragUniforms;
    SDL_GPUTexture* boundTexture = nullptr;
    bool surfacePushed = false;
    int drawn = 0, binds = 0;
    for (const Gta5DrawItem& item : batch.items) {
        const Gta5SubMesh& sub = *item.sub;
        // A submesh with no texture of its own falls back to the package
        // default rather than being dropped.
        SDL_GPUTexture* texture = sub.texture ? sub.texture : draw.texture;
        SDL_GPUSampler* sampler = sub.texture ? sub.sampler : draw.sampler;
        if (!texture || !sampler) continue;
        if (texture != boundTexture) {
            SDL_GPUTextureSamplerBinding binding = {texture, sampler};
            SDL_BindGPUFragmentSamplers(draw.pass, 0, &binding, 1);
            boundTexture = texture;
            ++binds;
        }
        SDL_GPUBufferBinding vb = {sub.vertexBuffer, 0};
        SDL_BindGPUVertexBuffers(draw.pass, 0, &vb, 1);
        SDL_GPUBufferBinding ib = {sub.indexBuffer, 0};
        SDL_BindGPUIndexBuffer(draw.pass, &ib, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        // The fragment shader reads the spotlight slot as the tint and
        // alpha threshold; the sort keeps equal ones together.
        if (!surfacePushed || std::memcmp(fu.flash_color, sub.surface.data(),
                                          sizeof(fu.flash_color)) != 0) {
            std::memcpy(fu.flash_color, sub.surface.data(),
                        sizeof(fu.flash_color));
            SDL_PushGPUFragmentUniformData(draw.cmd, 0, &fu, sizeof(fu));
            surfacePushed = true;
        }
        SDL_DrawGPUIndexedPrimitives(draw.pass, sub.indexCount, item.count, 0,
                                     0, item.first);
        ++drawn;
    }
    if (textureBinds) *textureBinds = binds;
    return drawn;
}

}  // namespace sdl3cpp::services::impl

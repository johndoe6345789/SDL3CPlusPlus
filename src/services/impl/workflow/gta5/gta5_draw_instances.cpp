#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

#include "services/interfaces/workflow/gta5/gta5_draw_bind.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw, int* textureBinds) {
    const Gta5InstanceBatch& batch = state.batch;
    if (!draw.pass || !draw.cmd || !batch.buffer || batch.items.empty()) {
        return 0;
    }
    BindGta5BatchShared(state, draw);
    rendering::FragmentUniformData fu = draw.fragUniforms;
    SDL_GPUTexture* boundTexture = nullptr;
    int boundBlock = -1;
    bool surfacePushed = false, blending = false;
    int drawn = 0, binds = 0;
    for (const Gta5DrawItem& item : batch.items) {
        const Gta5SubMesh& sub = *item.sub;
        if (sub.slot.block < 0) continue;
        if (sub.blend && !blending) {
            // Blended surfaces sort last: switch once, writing no depth,
            // and rebind what the pipeline change dropped.
            if (!draw.blendPipeline) break;
            SDL_BindGPUGraphicsPipeline(draw.pass, draw.blendPipeline);
            BindGta5BatchShared(state, draw);
            blending = true;
            boundTexture = nullptr;
            boundBlock = -1;
            surfacePushed = false;
        }
        // A blended surface without its own texture would lay the grey
        // default over the road as a slab: skip it.
        if (sub.blend && !sub.texture) continue;
        SDL_GPUTexture* texture = sub.texture ? sub.texture : draw.texture;
        SDL_GPUSampler* sampler = sub.texture ? sub.sampler : draw.sampler;
        if (!texture || !sampler) continue;
        if (sub.slot.block != boundBlock) {
            BindGta5ArenaBlock(state, draw, sub.slot.block);
            boundBlock = sub.slot.block;
        }
        if (texture != boundTexture) {
            SDL_GPUTextureSamplerBinding binding = {texture, sampler};
            SDL_BindGPUFragmentSamplers(draw.pass, 0, &binding, 1);
            boundTexture = texture;
            ++binds;
        }
        // Tint and alpha threshold ride in the spotlight slot.
        if (!surfacePushed || std::memcmp(fu.flash_color, sub.surface.data(),
                                          sizeof(fu.flash_color)) != 0) {
            std::memcpy(fu.flash_color, sub.surface.data(),
                        sizeof(fu.flash_color));
            SDL_PushGPUFragmentUniformData(draw.cmd, 0, &fu, sizeof(fu));
            surfacePushed = true;
        }
        SDL_DrawGPUIndexedPrimitives(
            draw.pass, sub.indexCount, item.count, sub.slot.firstIndex,
            static_cast<Sint32>(sub.slot.vertexOffset), item.first);
        ++drawn;
    }
    if (textureBinds) *textureBinds = binds;
    return drawn;
}

}  // namespace sdl3cpp::services::impl

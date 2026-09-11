#include "services/interfaces/workflow/gta5/gta5_draw_instances.hpp"

#include "services/interfaces/workflow/gta5/gta5_draw_bind.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

int DrawGta5Instances(const Gta5StreamState& state,
                      const Gta5DrawContext& draw, int* textureBinds,
                      const Gta5InstanceBatch* only) {
    const Gta5InstanceBatch& batch = only ? *only : state.batch;
    if (!draw.pass || !draw.cmd || !batch.buffer || batch.items.empty()) {
        return 0;
    }
    BindGta5BatchShared(draw, batch);
    rendering::FragmentUniformData fu = draw.fragUniforms;
    SDL_GPUTexture* boundTexture = nullptr;
    int boundBlock = -1, boundKind = 0;
    bool surfacePushed = false;
    int drawn = 0, binds = 0;
    for (const Gta5DrawItem& item : batch.items) {
        const Gta5SubMesh& sub = *item.sub;
        if (sub.slot.block < 0) continue;
        const int kind = sub.DrawKind();
        if (kind != boundKind) {
            // Opaque, cutout, terrain, emissive, blended, as sorted: each
            // pipeline is bound once, and what a switch drops is bound again.
            SDL_GPUGraphicsPipeline* next =
                kind == 1   ? draw.cutoutPipeline
                : kind == 2 ? draw.terrainPipeline
                : kind == 3 ? draw.emissivePipeline
                            : draw.blendPipeline;
            if (!next) continue;
            SDL_BindGPUGraphicsPipeline(draw.pass, next);
            BindGta5BatchShared(draw, batch);
            boundKind = kind;
            boundTexture = nullptr;
            boundBlock = -1;
            surfacePushed = false;
        }
        // A blended surface without its own texture would lay the grey
        // default over the road as a slab: skip it.
        if (sub.blend && !sub.texture) continue;
        const int bound = BindGta5SubMeshTextures(draw, sub, boundTexture);
        if (bound < 0) continue;
        binds += bound;
        if (sub.slot.block != boundBlock) {
            BindGta5ArenaBlock(state, draw, sub.slot.block);
            boundBlock = sub.slot.block;
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

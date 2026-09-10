#include "services/interfaces/workflow/rendering/bsp_map_geometry_textures.hpp"

namespace sdl3cpp::services::impl {

BspMapTextures GatherBspMapTextures(WorkflowContext& context,
                                    const std::string& defaultTexture) {
    BspMapTextures out;
    out.defaultTex =
        context.Get<SDL_GPUTexture*>(defaultTexture + "_gpu", nullptr);
    out.defaultSamp =
        context.Get<SDL_GPUSampler*>(defaultTexture + "_sampler", nullptr);
    out.shadowTex =
        context.Get<SDL_GPUTexture*>("shadow_depth_texture", nullptr);
    out.shadowSamp =
        context.Get<SDL_GPUSampler*>("shadow_depth_sampler", nullptr);
    out.lightmapTex =
        context.Get<SDL_GPUTexture*>("bsp_lightmap_atlas_gpu", nullptr);
    out.lightmapSamp =
        context.Get<SDL_GPUSampler*>("bsp_lightmap_atlas_sampler", nullptr);
    out.portalTex =
        context.Get<SDL_GPUTexture*>("bsp_portal_view_texture", nullptr);
    out.portalSamp =
        context.Get<SDL_GPUSampler*>("bsp_portal_view_sampler", nullptr);
    return out;
}

void ResolveBspGroupAlbedo(WorkflowContext& context, int texIdx,
                           const BspMapTextures& textures,
                           SDL_GPUTexture*& outTex, SDL_GPUSampler*& outSamp) {
    outTex  = nullptr;
    outSamp = nullptr;
    if (texIdx >= 0) {
        std::string texKey = "bsp_tex_" + std::to_string(texIdx);
        outTex  = context.Get<SDL_GPUTexture*>(texKey + "_gpu", nullptr);
        outSamp = context.Get<SDL_GPUSampler*>(texKey + "_sampler", nullptr);
    }
    if (!outTex || !outSamp) {
        outTex  = textures.defaultTex;
        outSamp = textures.defaultSamp;
    }
}

void BuildBspSamplerBindings(SDL_GPUTexture* albedoTex,
                             SDL_GPUSampler* albedoSamp,
                             const BspMapTextures& textures,
                             SDL_GPUTextureSamplerBinding outBindings[4]) {
    outBindings[0].texture = albedoTex;
    outBindings[0].sampler = albedoSamp;
    outBindings[1].texture =
        textures.shadowTex ? textures.shadowTex : albedoTex;
    outBindings[1].sampler =
        textures.shadowSamp ? textures.shadowSamp : albedoSamp;
    outBindings[2].texture =
        textures.lightmapTex ? textures.lightmapTex : albedoTex;
    outBindings[2].sampler =
        textures.lightmapSamp ? textures.lightmapSamp : albedoSamp;
    outBindings[3].texture =
        textures.portalTex ? textures.portalTex : albedoTex;
    outBindings[3].sampler =
        textures.portalSamp ? textures.portalSamp : albedoSamp;
}

}  // namespace sdl3cpp::services::impl

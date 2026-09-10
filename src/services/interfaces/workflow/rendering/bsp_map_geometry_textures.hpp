#pragma once

#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

/// The samplers shared by every BSP texture group in one draw.map call:
/// the fallback albedo plus the shadow/lightmap/portal-destination textures
/// (each may be null if the pass hasn't produced it, in which case the
/// caller substitutes the group's own albedo).
struct BspMapTextures {
    SDL_GPUTexture* defaultTex   = nullptr;
    SDL_GPUSampler* defaultSamp  = nullptr;
    SDL_GPUTexture* shadowTex    = nullptr;
    SDL_GPUSampler* shadowSamp   = nullptr;
    SDL_GPUTexture* lightmapTex  = nullptr;
    SDL_GPUSampler* lightmapSamp = nullptr;
    SDL_GPUTexture* portalTex    = nullptr;
    SDL_GPUSampler* portalSamp   = nullptr;
};

/// Looks up the fallback albedo plus the shared shadow/lightmap/portal
/// textures once per draw.map call, so the per-group loop only has to
/// resolve each group's own albedo texture.
BspMapTextures GatherBspMapTextures(WorkflowContext& context,
                                    const std::string& defaultTexture);

/// Resolves a BSP texture group's albedo, falling back to
/// `textures.defaultTex/defaultSamp` when the group has no texture index or
/// no loaded GPU texture for it.
void ResolveBspGroupAlbedo(WorkflowContext& context, int texIdx,
                           const BspMapTextures& textures,
                           SDL_GPUTexture*& outTex, SDL_GPUSampler*& outSamp);

/// Fills the 4 fragment-sampler slots (albedo, shadow, lightmap, portal
/// destination) for one draw call, substituting `albedoTex`/`albedoSamp`
/// for any of the shared textures that isn't available this frame.
void BuildBspSamplerBindings(SDL_GPUTexture* albedoTex,
                             SDL_GPUSampler* albedoSamp,
                             const BspMapTextures& textures,
                             SDL_GPUTextureSamplerBinding outBindings[4]);

}  // namespace sdl3cpp::services::impl

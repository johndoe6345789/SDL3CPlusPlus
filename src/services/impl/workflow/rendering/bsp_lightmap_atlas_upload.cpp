#include "services/interfaces/workflow/rendering/bsp_lightmap_atlas_upload.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

LightmapAtlasGpu UploadLightmapAtlas(SDL_GPUDevice* device,
                                     const LightmapAtlas& atlas) {
    LightmapAtlasGpu out;

    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = static_cast<Uint32>(atlas.width);
    ti.height                   = static_cast<Uint32>(atlas.height);
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = 1;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    out.texture                 = SDL_CreateGPUTexture(device, &ti);

    Uint32 dataSize = static_cast<Uint32>(atlas.pixels.size());
    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = dataSize;
    auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
    auto* mapped =
        static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    std::memcpy(mapped, atlas.pixels.data(), dataSize);
    SDL_UnmapGPUTransferBuffer(device, tb);

    auto* cmd                          = SDL_AcquireGPUCommandBuffer(device);
    auto* cp                           = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo srcInfo = {};
    srcInfo.transfer_buffer            = tb;
    SDL_GPUTextureRegion dstRegion     = {};
    dstRegion.texture                  = out.texture;
    dstRegion.w                        = static_cast<Uint32>(atlas.width);
    dstRegion.h                        = static_cast<Uint32>(atlas.height);
    dstRegion.d                        = 1;
    SDL_UploadToGPUTexture(cp, &srcInfo, &dstRegion, false);
    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    out.sampler                 = SDL_CreateGPUSampler(device, &si);

    return out;
}

}  // namespace sdl3cpp::services::impl

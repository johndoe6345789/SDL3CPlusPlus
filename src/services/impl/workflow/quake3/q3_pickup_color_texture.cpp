#include "services/interfaces/workflow/quake3/q3_pickup_color_texture.hpp"

namespace sdl3cpp::services::impl {

SDL_GPUTexture* EnsurePickupColorTexture(SDL_GPUDevice* device,
                                         WorkflowContext& context,
                                         const std::string& key, uint8_t r,
                                         uint8_t g, uint8_t b) {
    auto* existing = context.Get<SDL_GPUTexture*>(key + "_gpu", nullptr);
    if (existing) return existing;

    SDL_GPUTextureCreateInfo ti = {};
    ti.type                     = SDL_GPU_TEXTURETYPE_2D;
    ti.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    ti.width                    = 1;
    ti.height                   = 1;
    ti.layer_count_or_depth     = 1;
    ti.num_levels               = 1;
    ti.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    auto* tex                   = SDL_CreateGPUTexture(device, &ti);
    if (!tex) return nullptr;

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = 4;
    auto* tb = SDL_CreateGPUTransferBuffer(device, &tbi);
    auto* mapped =
        static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    mapped[0] = r;
    mapped[1] = g;
    mapped[2] = b;
    mapped[3] = 230;
    SDL_UnmapGPUTransferBuffer(device, tb);

    auto* cmd                      = SDL_AcquireGPUCommandBuffer(device);
    auto* copy                     = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTextureTransferInfo src = {};
    src.transfer_buffer            = tb;
    SDL_GPUTextureRegion dst       = {};
    dst.texture                    = tex;
    dst.w                          = 1;
    dst.h                          = 1;
    dst.d                          = 1;
    SDL_UploadToGPUTexture(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);

    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_NEAREST;
    si.mag_filter               = SDL_GPU_FILTER_NEAREST;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    auto* sampler               = SDL_CreateGPUSampler(device, &si);
    context.Set<SDL_GPUTexture*>(key + "_gpu", tex);
    context.Set<SDL_GPUSampler*>(key + "_sampler", sampler);
    return tex;
}

}  // namespace sdl3cpp::services::impl

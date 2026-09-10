#include "services/interfaces/workflow/rendering/overlay_sw_end_resources_internal.hpp"

namespace sdl3cpp::services::impl::overlay_sw_end_detail {

bool CreateGpuBuffers(SDL_GPUDevice* device, int surfaceWidth,
                      int surfaceHeight, OverlaySwEndResources& res) {
    SDL_GPUTextureCreateInfo tci = {};
    tci.type                     = SDL_GPU_TEXTURETYPE_2D;
    tci.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.width                    = static_cast<uint32_t>(surfaceWidth);
    tci.height                   = static_cast<uint32_t>(surfaceHeight);
    tci.layer_count_or_depth     = 1;
    tci.num_levels               = 1;
    tci.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    res.texture                  = SDL_CreateGPUTexture(device, &tci);

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size    = static_cast<uint32_t>(surfaceWidth * surfaceHeight * 4);
    res.transfer = SDL_CreateGPUTransferBuffer(device, &tbci);

    SDL_GPUBufferCreateInfo bci = {};
    bci.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size     = 6u * 5u * static_cast<uint32_t>(sizeof(float));
    res.vertices = SDL_CreateGPUBuffer(device, &bci);

    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    res.sampler                  = SDL_CreateGPUSampler(device, &sci);

    return res.texture && res.transfer && res.vertices && res.sampler;
}

}  // namespace sdl3cpp::services::impl::overlay_sw_end_detail

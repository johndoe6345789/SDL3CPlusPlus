#include "services/interfaces/workflow/rendering/gpu_text_overlay_buffer_factory.hpp"

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUTexture* CreateOverlayTexture(SDL_GPUDevice* device) {
    SDL_GPUTextureCreateInfo tci = {};
    tci.type                     = SDL_GPU_TEXTURETYPE_2D;
    tci.format                   = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    tci.usage                    = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    tci.width                    = kGpuTextOverlayWidth;
    tci.height                   = kGpuTextOverlayHeight;
    tci.layer_count_or_depth     = 1;
    tci.num_levels               = 1;
    return SDL_CreateGPUTexture(device, &tci);
}

SDL_GPUSampler* CreateOverlaySampler(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo sci = {};
    sci.min_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mag_filter               = SDL_GPU_FILTER_NEAREST;
    sci.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    sci.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    sci.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &sci);
}

}  // namespace

const char* CreateOverlayGpuBuffers(SDL_GPUDevice* device,
                                    GpuTextOverlayResources& res) {
    res.texture = CreateOverlayTexture(device);
    if (!res.texture) {
        return "overlay texture creation failed";
    }

    SDL_GPUTransferBufferCreateInfo tbci = {};
    tbci.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbci.size =
        static_cast<Uint32>(kGpuTextOverlayWidth * kGpuTextOverlayHeight * 4);
    res.transfer = SDL_CreateGPUTransferBuffer(device, &tbci);
    if (!res.transfer) {
        return "overlay transfer buffer creation failed";
    }

    SDL_GPUBufferCreateInfo bci = {};
    bci.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    bci.size                    = 6u * 5u * static_cast<Uint32>(sizeof(float));
    res.vertices                = SDL_CreateGPUBuffer(device, &bci);
    if (!res.vertices) {
        return "overlay vertex buffer creation failed";
    }

    res.sampler = CreateOverlaySampler(device);
    if (!res.sampler) {
        return "overlay sampler creation failed";
    }

    return "";
}

}  // namespace sdl3cpp::services::impl

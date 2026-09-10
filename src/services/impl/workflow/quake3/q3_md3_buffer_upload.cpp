#include "services/interfaces/workflow/quake3/q3_md3_gpu_upload.hpp"

#include <cstring>

namespace sdl3cpp::q3 {

SDL_GPUBuffer* UploadMd3Buffer(SDL_GPUDevice* device,
                               SDL_GPUBufferUsageFlags usage, const void* data,
                               uint32_t size) {
    SDL_GPUBufferCreateInfo bi = {};
    bi.usage                   = usage;
    bi.size                    = size;
    auto* buffer               = SDL_CreateGPUBuffer(device, &bi);
    if (!buffer) {
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo tbi = {};
    tbi.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbi.size                            = size;
    auto* staging = SDL_CreateGPUTransferBuffer(device, &tbi);
    if (!staging) {
        SDL_ReleaseGPUBuffer(device, buffer);
        return nullptr;
    }

    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        std::memcpy(mapped, data, size);
        SDL_UnmapGPUTransferBuffer(device, staging);
    }

    auto* cmd                         = SDL_AcquireGPUCommandBuffer(device);
    auto* copy                        = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = {staging, 0};
    SDL_GPUBufferRegion dst           = {buffer, 0, size};
    SDL_UploadToGPUBuffer(copy, &src, &dst, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, staging);
    return buffer;
}

SDL_GPUSampler* MakeMd3LinearSampler(SDL_GPUDevice* device) {
    SDL_GPUSamplerCreateInfo si = {};
    si.min_filter               = SDL_GPU_FILTER_LINEAR;
    si.mag_filter               = SDL_GPU_FILTER_LINEAR;
    si.mipmap_mode              = SDL_GPU_SAMPLERMIPMAPMODE_LINEAR;
    si.address_mode_u           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_v           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    si.address_mode_w           = SDL_GPU_SAMPLERADDRESSMODE_REPEAT;
    return SDL_CreateGPUSampler(device, &si);
}

}  // namespace sdl3cpp::q3

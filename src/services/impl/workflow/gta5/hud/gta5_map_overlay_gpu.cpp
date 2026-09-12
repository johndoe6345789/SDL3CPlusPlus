#include "services/interfaces/workflow/gta5/hud/gta5_map_overlay.hpp"

namespace sdl3cpp::services::impl {
namespace {

SDL_GPUSampler* Sampler(SDL_GPUDevice* device, SDL_GPUFilter filter) {
    SDL_GPUSamplerCreateInfo info = {};
    info.min_filter = filter;
    info.mag_filter = filter;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler(device, &info);
}

}  // namespace

bool CreateGta5MapGpu(Gta5MapOverlay& map, SDL_GPUDevice* device) {
    const auto bytes =
        static_cast<Uint32>(kGta5MapMaxQuads * 6 * 5 * sizeof(float));
    SDL_GPUBufferCreateInfo buffer = {};
    buffer.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer.size = bytes;
    map.vertices = SDL_CreateGPUBuffer(device, &buffer);
    SDL_GPUTransferBufferCreateInfo staging = {};
    staging.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    staging.size = bytes;
    map.staging = SDL_CreateGPUTransferBuffer(device, &staging);
    map.sampler = Sampler(device, SDL_GPU_FILTER_LINEAR);
    map.nearest = Sampler(device, SDL_GPU_FILTER_NEAREST);
    return map.vertices && map.staging && map.sampler && map.nearest;
}

}  // namespace sdl3cpp::services::impl

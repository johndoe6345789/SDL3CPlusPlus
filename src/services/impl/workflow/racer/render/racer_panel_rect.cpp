#include "services/interfaces/workflow/racer/render/racer_panel.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

/// Copies `bytes` through a fresh transfer buffer into `dst`.
void UploadBytes(SDL_GPUDevice* device, SDL_GPUCommandBuffer* cmd,
                 const void* bytes, Uint32 size, SDL_GPUBuffer* dst) {
    SDL_GPUTransferBufferCreateInfo info = {};
    info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    info.size = size;
    SDL_GPUTransferBuffer* staging = SDL_CreateGPUTransferBuffer(device, &info);
    if (!staging) return;
    if (void* mapped = SDL_MapGPUTransferBuffer(device, staging, false)) {
        std::memcpy(mapped, bytes, size);
        SDL_UnmapGPUTransferBuffer(device, staging);
        if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(cmd)) {
            SDL_GPUTransferBufferLocation src = {staging, 0};
            SDL_GPUBufferRegion region = {dst, 0, size};
            SDL_UploadToGPUBuffer(copy, &src, &region, false);
            SDL_EndGPUCopyPass(copy);
        }
    }
    SDL_ReleaseGPUTransferBuffer(device, staging);
}

}  // namespace

void UploadRacerPanelRect(RacerPanel& panel, SDL_GPUCommandBuffer* cmd,
                          const RacerScreenRect& r) {
    if (!cmd || !panel.device) return;
    if (!panel.quad) {
        SDL_GPUBufferCreateInfo bci = {};
        bci.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
        bci.size = 6 * 5 * sizeof(float);
        panel.quad = SDL_CreateGPUBuffer(panel.device, &bci);
        if (!panel.quad) return;
    }
    const float quad[6 * 5] = {
        r.left,  r.top,    0.f, r.u0, r.v0, r.right, r.top,    0.f, r.u1, r.v0,
        r.right, r.bottom, 0.f, r.u1, r.v1, r.left,  r.top,    0.f, r.u0, r.v0,
        r.right, r.bottom, 0.f, r.u1, r.v1, r.left,  r.bottom, 0.f, r.u0, r.v1};
    UploadBytes(panel.device, cmd, quad, sizeof(quad), panel.quad);
    panel.quadUploaded = true;
}

}  // namespace sdl3cpp::services::impl

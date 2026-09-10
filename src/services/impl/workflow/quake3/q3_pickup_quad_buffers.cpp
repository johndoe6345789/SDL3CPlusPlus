#include "services/interfaces/workflow/quake3/q3_pickup_quad_buffers.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

void EnsurePickupQuadBuffers(SDL_GPUDevice* device,
                             PickupQuadBuffers& buffers) {
    if (buffers.quadVb && buffers.quadIb) return;
    buffers.device = device;
    struct V {
        float x, y, z, u, v;
    };
    const V verts[4] = {
        {-0.5f, -0.5f, 0.0f, 0.0f, 1.0f},
        {0.5f, -0.5f, 0.0f, 1.0f, 1.0f},
        {0.5f, 0.5f, 0.0f, 1.0f, 0.0f},
        {-0.5f, 0.5f, 0.0f, 0.0f, 0.0f},
    };
    const uint16_t indices[6] = {0, 1, 2, 0, 2, 3};
    const uint32_t total      = sizeof(verts) + sizeof(indices);

    SDL_GPUBufferCreateInfo vbInfo = {};
    vbInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size                    = sizeof(verts);
    buffers.quadVb                 = SDL_CreateGPUBuffer(device, &vbInfo);
    SDL_GPUBufferCreateInfo ibInfo = {};
    ibInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibInfo.size                    = sizeof(indices);
    buffers.quadIb                 = SDL_CreateGPUBuffer(device, &ibInfo);
    SDL_GPUTransferBufferCreateInfo tbInfo = {};
    tbInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbInfo.size                            = total;
    buffers.transfer = SDL_CreateGPUTransferBuffer(device, &tbInfo);
    auto* mapped     = static_cast<uint8_t*>(
        SDL_MapGPUTransferBuffer(device, buffers.transfer, false));
    std::memcpy(mapped, verts, sizeof(verts));
    std::memcpy(mapped + sizeof(verts), indices, sizeof(indices));
    SDL_UnmapGPUTransferBuffer(device, buffers.transfer);

    auto* cmd                          = SDL_AcquireGPUCommandBuffer(device);
    auto* copy                         = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation srcV = {buffers.transfer, 0};
    SDL_GPUBufferRegion dstV           = {buffers.quadVb, 0, sizeof(verts)};
    SDL_UploadToGPUBuffer(copy, &srcV, &dstV, false);
    SDL_GPUTransferBufferLocation srcI = {buffers.transfer, sizeof(verts)};
    SDL_GPUBufferRegion dstI           = {buffers.quadIb, 0, sizeof(indices)};
    SDL_UploadToGPUBuffer(copy, &srcI, &dstI, false);
    SDL_EndGPUCopyPass(copy);
    SDL_SubmitGPUCommandBuffer(cmd);
}

void ReleasePickupQuadBuffers(PickupQuadBuffers& buffers) {
    if (!buffers.device) return;
    if (buffers.quadVb) SDL_ReleaseGPUBuffer(buffers.device, buffers.quadVb);
    if (buffers.quadIb) SDL_ReleaseGPUBuffer(buffers.device, buffers.quadIb);
    if (buffers.transfer) {
        SDL_ReleaseGPUTransferBuffer(buffers.device, buffers.transfer);
    }
}

}  // namespace sdl3cpp::services::impl

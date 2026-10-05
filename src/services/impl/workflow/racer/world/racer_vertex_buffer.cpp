#include "services/interfaces/workflow/racer/world/racer_gpu_upload.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

SDL_GPUBuffer* UploadRacerVertexBuffer(SDL_GPUDevice* device,
                                       const std::vector<RacerGpuVertex>& v) {
    if (!device || v.empty()) return nullptr;
    const auto size = static_cast<Uint32>(v.size() * sizeof(RacerGpuVertex));
    SDL_GPUBufferCreateInfo info = {};
    info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    info.size = size;
    SDL_GPUBuffer* buffer = SDL_CreateGPUBuffer(device, &info);
    SDL_GPUTransferBufferCreateInfo tinfo = {};
    tinfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tinfo.size = size;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tinfo);
    if (!buffer || !transfer) {
        if (buffer) SDL_ReleaseGPUBuffer(device, buffer);
        if (transfer) SDL_ReleaseGPUTransferBuffer(device, transfer);
        return nullptr;
    }
    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(mapped, v.data(), size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* pass = SDL_BeginGPUCopyPass(cmd);
    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer = transfer;
    SDL_GPUBufferRegion dst = {};
    dst.buffer = buffer;
    dst.size = size;
    SDL_UploadToGPUBuffer(pass, &src, &dst, false);
    SDL_EndGPUCopyPass(pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
    return buffer;
}

}  // namespace sdl3cpp::services::impl

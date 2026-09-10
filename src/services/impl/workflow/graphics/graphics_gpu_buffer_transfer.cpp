#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_transfer.hpp"

#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

void UploadGpuBufferData(SDL_GPUDevice* device,
                         const UploadedGpuBuffers& buffers,
                         const std::vector<uint8_t>& vertexBytes,
                         const std::vector<uint16_t>& indexValues) {
    const auto vertexSize = static_cast<uint32_t>(vertexBytes.size());
    const auto indexSize =
        static_cast<uint32_t>(indexValues.size() * sizeof(uint16_t));

    const uint32_t transferSize                  = vertexSize + indexSize;
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size  = transferSize;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer) {
        throw std::runtime_error(
            "graphics.buffer.upload: Failed to create transfer buffer");
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    std::memcpy(mapped, vertexBytes.data(), vertexSize);
    std::memcpy(static_cast<uint8_t*>(mapped) + vertexSize, indexValues.data(),
                indexSize);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation vSrc = {};
    vSrc.transfer_buffer               = transfer;
    vSrc.offset                        = 0;
    SDL_GPUBufferRegion vDst           = {};
    vDst.buffer                        = buffers.vertexBuffer;
    vDst.offset                        = 0;
    vDst.size                          = vertexSize;
    SDL_UploadToGPUBuffer(copyPass, &vSrc, &vDst, false);

    SDL_GPUTransferBufferLocation iSrc = {};
    iSrc.transfer_buffer               = transfer;
    iSrc.offset                        = vertexSize;
    SDL_GPUBufferRegion iDst           = {};
    iDst.buffer                        = buffers.indexBuffer;
    iDst.offset                        = 0;
    iDst.size                          = indexSize;
    SDL_UploadToGPUBuffer(copyPass, &iSrc, &iDst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
}

}  // namespace sdl3cpp::services::impl

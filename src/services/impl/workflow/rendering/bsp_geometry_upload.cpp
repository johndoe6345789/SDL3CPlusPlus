#include "services/interfaces/workflow/rendering/bsp_geometry_upload.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

BspGeometryBuffers UploadBspGeometryBuffers(
    SDL_GPUDevice* device, const std::vector<BspRenderVertex>& vertices,
    const std::vector<uint32_t>& indices) {
    const uint32_t vtxSize =
        static_cast<uint32_t>(vertices.size() * sizeof(BspRenderVertex));
    const uint32_t idxSize =
        static_cast<uint32_t>(indices.size() * sizeof(uint32_t));

    SDL_GPUBufferCreateInfo vbInfo = {};
    vbInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbInfo.size                    = vtxSize;
    SDL_GPUBuffer* vb              = SDL_CreateGPUBuffer(device, &vbInfo);

    SDL_GPUBufferCreateInfo ibInfo = {};
    ibInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibInfo.size                    = idxSize;
    SDL_GPUBuffer* ib              = SDL_CreateGPUBuffer(device, &ibInfo);

    SDL_GPUTransferBufferCreateInfo tbInfo = {};
    tbInfo.usage                           = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbInfo.size                            = vtxSize + idxSize;
    SDL_GPUTransferBuffer* tb = SDL_CreateGPUTransferBuffer(device, &tbInfo);

    auto* mapped =
        static_cast<uint8_t*>(SDL_MapGPUTransferBuffer(device, tb, false));
    std::memcpy(mapped, vertices.data(), vtxSize);
    std::memcpy(mapped + vtxSize, indices.data(), idxSize);
    SDL_UnmapGPUTransferBuffer(device, tb);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* cp       = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation srcV = {};
    srcV.transfer_buffer               = tb;
    SDL_GPUBufferRegion dstV           = {};
    dstV.buffer                        = vb;
    dstV.size                          = vtxSize;
    SDL_UploadToGPUBuffer(cp, &srcV, &dstV, false);

    SDL_GPUTransferBufferLocation srcI = {};
    srcI.transfer_buffer               = tb;
    srcI.offset                        = vtxSize;
    SDL_GPUBufferRegion dstI           = {};
    dstI.buffer                        = ib;
    dstI.size                          = idxSize;
    SDL_UploadToGPUBuffer(cp, &srcI, &dstI, false);

    SDL_EndGPUCopyPass(cp);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, tb);

    return BspGeometryBuffers{vb, ib};
}

}  // namespace sdl3cpp::services::impl

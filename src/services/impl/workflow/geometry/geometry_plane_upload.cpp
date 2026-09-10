#include "services/interfaces/workflow/geometry/geometry_plane_upload_internal.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

GeometryPlaneBuffers UploadGeometryPlaneMesh(SDL_GPUDevice* device,
                                             const GeometryPlaneMesh& mesh) {
    SDL_GPUTransferBuffer* transfer = nullptr;
    GeometryPlaneBuffers result =
        geometry_plane_detail::CreateMeshBuffers(device, mesh, transfer);
    const uint32_t vertexSize =
        static_cast<uint32_t>(mesh.vertices.size()) *
        sizeof(PlanePosUvVertex);
    const uint32_t indexSize =
        static_cast<uint32_t>(mesh.indices.size()) * sizeof(uint16_t);

    auto* mapped = static_cast<uint8_t*>(
        SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, mesh.vertices.data(), vertexSize);
    std::memcpy(mapped + vertexSize, mesh.indices.data(), indexSize);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation srcVert = {};
    srcVert.transfer_buffer               = transfer;
    srcVert.offset                        = 0;
    SDL_GPUBufferRegion dstVert           = {};
    dstVert.buffer                        = result.vertexBuffer;
    dstVert.offset                        = 0;
    dstVert.size                          = vertexSize;
    SDL_UploadToGPUBuffer(copyPass, &srcVert, &dstVert, false);

    SDL_GPUTransferBufferLocation srcIdx = {};
    srcIdx.transfer_buffer               = transfer;
    srcIdx.offset                        = vertexSize;
    SDL_GPUBufferRegion dstIdx           = {};
    dstIdx.buffer                        = result.indexBuffer;
    dstIdx.offset                        = 0;
    dstIdx.size                          = indexSize;
    SDL_UploadToGPUBuffer(copyPass, &srcIdx, &dstIdx, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return result;
}

}  // namespace sdl3cpp::services::impl

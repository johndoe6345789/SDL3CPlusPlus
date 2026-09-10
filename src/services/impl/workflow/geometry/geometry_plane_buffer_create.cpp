#include "services/interfaces/workflow/geometry/geometry_plane_upload_internal.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl::geometry_plane_detail {

GeometryPlaneBuffers CreateMeshBuffers(SDL_GPUDevice* device,
                                       const GeometryPlaneMesh& mesh,
                                       SDL_GPUTransferBuffer*& outTransfer) {
    const auto vertexCount    = static_cast<uint32_t>(mesh.vertices.size());
    const auto indexCount     = static_cast<uint32_t>(mesh.indices.size());
    const uint32_t vertexSize = vertexCount * sizeof(PlanePosUvVertex);
    const uint32_t indexSize  = indexCount * sizeof(uint16_t);

    SDL_GPUBufferCreateInfo vbufInfo = {};
    vbufInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbufInfo.size                    = vertexSize;
    SDL_GPUBuffer* vertexBuffer      = SDL_CreateGPUBuffer(device, &vbufInfo);
    if (!vertexBuffer) {
        throw std::runtime_error(
            "geometry.create_plane: Failed to create vertex buffer");
    }

    SDL_GPUBufferCreateInfo ibufInfo = {};
    ibufInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibufInfo.size                    = indexSize;
    SDL_GPUBuffer* indexBuffer       = SDL_CreateGPUBuffer(device, &ibufInfo);
    if (!indexBuffer) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        throw std::runtime_error(
            "geometry.create_plane: Failed to create index buffer");
    }

    SDL_GPUTransferBufferCreateInfo tbufInfo = {};
    tbufInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbufInfo.size  = vertexSize + indexSize;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbufInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vertexBuffer);
        SDL_ReleaseGPUBuffer(device, indexBuffer);
        throw std::runtime_error(
            "geometry.create_plane: Failed to create transfer buffer");
    }

    outTransfer = transfer;
    GeometryPlaneBuffers result;
    result.vertexBuffer = vertexBuffer;
    result.indexBuffer  = indexBuffer;
    return result;
}

}  // namespace sdl3cpp::services::impl::geometry_plane_detail

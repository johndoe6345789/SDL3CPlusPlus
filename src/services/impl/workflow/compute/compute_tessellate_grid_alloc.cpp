#include "services/interfaces/workflow/compute/compute_tessellate_grid_internal.hpp"

#include <stdexcept>

namespace sdl3cpp::services::impl::tessellate_grid_detail {

TessellationGridBuffers AllocateGridBuffers(SDL_GPUDevice* device,
                                            int subdivisions) {
    TessellationGridBuffers buffers;
    const uint32_t vertsPerSide = static_cast<uint32_t>(subdivisions + 1);
    buffers.vertexCount         = vertsPerSide * vertsPerSide;
    buffers.indexCount = static_cast<uint32_t>(subdivisions * subdivisions * 6);

    SDL_GPUBufferCreateInfo vbufInfo = {};
    vbufInfo.usage =
        SDL_GPU_BUFFERUSAGE_VERTEX | SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_WRITE;
    vbufInfo.size        = buffers.vertexCount * buffers.vertexStride;
    buffers.vertexBuffer = SDL_CreateGPUBuffer(device, &vbufInfo);
    if (!buffers.vertexBuffer) {
        throw std::runtime_error(
            "compute.tessellate: Failed to create vertex buffer");
    }

    const uint32_t indexSize         = buffers.indexCount * sizeof(uint16_t);
    SDL_GPUBufferCreateInfo ibufInfo = {};
    ibufInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibufInfo.size                    = indexSize;
    buffers.indexBuffer              = SDL_CreateGPUBuffer(device, &ibufInfo);
    if (!buffers.indexBuffer) {
        SDL_ReleaseGPUBuffer(device, buffers.vertexBuffer);
        throw std::runtime_error(
            "compute.tessellate: Failed to create index buffer");
    }

    return buffers;
}

}  // namespace sdl3cpp::services::impl::tessellate_grid_detail

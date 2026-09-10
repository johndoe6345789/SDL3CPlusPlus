#include "services/interfaces/workflow/compute/compute_tessellate_grid_internal.hpp"

#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl::tessellate_grid_detail {
namespace {

std::vector<uint16_t> GenerateGridIndices(int subdivisions,
                                          uint32_t vertsPerSide) {
    std::vector<uint16_t> indices;
    indices.reserve(static_cast<size_t>(subdivisions) * subdivisions * 6);
    for (int iy = 0; iy < subdivisions; ++iy) {
        for (int ix = 0; ix < subdivisions; ++ix) {
            const uint16_t tl = static_cast<uint16_t>(iy * vertsPerSide + ix);
            const uint16_t tr = tl + 1;
            const uint16_t bl =
                static_cast<uint16_t>((iy + 1) * vertsPerSide + ix);
            const uint16_t br = bl + 1;
            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);
            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }
    return indices;
}

}  // namespace

void UploadGridIndices(SDL_GPUDevice* device, int subdivisions,
                       const TessellationGridBuffers& buffers) {
    const uint32_t vertsPerSide = static_cast<uint32_t>(subdivisions + 1);
    const uint32_t indexSize    = buffers.indexCount * sizeof(uint16_t);

    SDL_GPUTransferBufferCreateInfo tbufInfo = {};
    tbufInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbufInfo.size  = indexSize;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbufInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, buffers.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, buffers.indexBuffer);
        throw std::runtime_error(
            "compute.tessellate: Failed to create transfer buffer");
    }

    const std::vector<uint16_t> indices =
        GenerateGridIndices(subdivisions, vertsPerSide);
    if (void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false)) {
        std::memcpy(mapped, indices.data(), indexSize);
        SDL_UnmapGPUTransferBuffer(device, transfer);
    }

    SDL_GPUCommandBuffer* uploadCmd = SDL_AcquireGPUCommandBuffer(device);
    if (SDL_GPUCopyPass* copy = SDL_BeginGPUCopyPass(uploadCmd)) {
        SDL_GPUTransferBufferLocation src = {transfer, 0};
        SDL_GPUBufferRegion dst           = {buffers.indexBuffer, 0, indexSize};
        SDL_UploadToGPUBuffer(copy, &src, &dst, false);
        SDL_EndGPUCopyPass(copy);
    }
    SDL_SubmitGPUCommandBuffer(uploadCmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);
}

}  // namespace sdl3cpp::services::impl::tessellate_grid_detail

#include "services/interfaces/workflow/rendering/flashlight_mesh.hpp"

#include <cstring>

namespace sdl3cpp::services::impl {

FlashlightMeshBuffers UploadFlashlightMesh(SDL_GPUDevice* device,
                                           const FlashlightMesh& mesh) {
    const uint32_t vertex_size =
        static_cast<uint32_t>(mesh.vertices.size() * sizeof(PosUvVertex));
    const uint32_t index_size =
        static_cast<uint32_t>(mesh.indices.size() * sizeof(uint16_t));

    SDL_GPUBufferCreateInfo vbuf_info = {};
    vbuf_info.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbuf_info.size                    = vertex_size;
    SDL_GPUBuffer* vertex_buffer      = SDL_CreateGPUBuffer(device, &vbuf_info);

    SDL_GPUBufferCreateInfo ibuf_info = {};
    ibuf_info.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibuf_info.size                    = index_size;
    SDL_GPUBuffer* index_buffer       = SDL_CreateGPUBuffer(device, &ibuf_info);

    SDL_GPUTransferBufferCreateInfo tbuf_info = {};
    tbuf_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    tbuf_info.size  = vertex_size + index_size;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &tbuf_info);

    auto* mapped = static_cast<uint8_t*>(
        SDL_MapGPUTransferBuffer(device, transfer, false));
    std::memcpy(mapped, mesh.vertices.data(), vertex_size);
    std::memcpy(mapped + vertex_size, mesh.indices.data(), index_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src_vert = {};
    src_vert.transfer_buffer               = transfer;
    SDL_GPUBufferRegion dst_vert           = {};
    dst_vert.buffer                        = vertex_buffer;
    dst_vert.size                          = vertex_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_vert, &dst_vert, false);

    SDL_GPUTransferBufferLocation src_idx = {};
    src_idx.transfer_buffer               = transfer;
    src_idx.offset                        = vertex_size;
    SDL_GPUBufferRegion dst_idx           = {};
    dst_idx.buffer                        = index_buffer;
    dst_idx.size                          = index_size;
    SDL_UploadToGPUBuffer(copy_pass, &src_idx, &dst_idx, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return {vertex_buffer, index_buffer};
}

}  // namespace sdl3cpp::services::impl

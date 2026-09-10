#include "services/interfaces/workflow/graphics/graphics_buffer_create_vertex_helpers.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

std::vector<float> ParseVertexFloats(const nlohmann::json& verticesJson) {
    std::vector<float> vertex_data;
    for (const auto& v : verticesJson) {
        if (v.is_number()) {
            vertex_data.push_back(v.get<float>());
        } else {
            throw std::runtime_error(
                "graphics.buffer.create_vertex: all vertices must be "
                "numbers");
        }
    }
    return vertex_data;
}

SDL_GPUBuffer* UploadVertexBuffer(SDL_GPUDevice* device,
                                  const std::vector<float>& data) {
    const uint32_t data_size =
        static_cast<uint32_t>(data.size() * sizeof(float));

    SDL_GPUBufferCreateInfo buf_info = {};
    buf_info.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    buf_info.size                    = data_size;
    SDL_GPUBuffer* vbuf              = SDL_CreateGPUBuffer(device, &buf_info);
    if (!vbuf) {
        throw std::runtime_error(
            "graphics.buffer.create_vertex: SDL_CreateGPUBuffer failed: " +
            std::string(SDL_GetError()));
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size  = data_size;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vbuf);
        throw std::runtime_error(
            "graphics.buffer.create_vertex: Failed to create transfer "
            "buffer");
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    memcpy(mapped, data.data(), data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer               = transfer;
    src.offset                        = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer              = vbuf;
    dst.offset              = 0;
    dst.size                = data_size;

    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    // Release transfer buffer (GPU buffer persists).
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return vbuf;
}

}  // namespace sdl3cpp::services::impl

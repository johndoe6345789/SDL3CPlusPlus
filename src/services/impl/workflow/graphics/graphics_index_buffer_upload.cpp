#include "services/interfaces/workflow/graphics/graphics_index_buffer_upload.hpp"

#include <cstring>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

std::vector<uint16_t> ExtractIndexData(const nlohmann::json& indices_json) {
    std::vector<uint16_t> index_data;
    for (const auto& idx : indices_json) {
        if (!idx.is_number()) {
            throw std::runtime_error(
                "graphics.buffer.create_index: all indices must be numbers");
        }
        index_data.push_back(static_cast<uint16_t>(idx.get<int>()));
    }

    if (index_data.empty()) {
        throw std::runtime_error(
            "graphics.buffer.create_index: indices array is empty");
    }
    return index_data;
}

IndexBufferUploadResult CreateAndUploadIndexBuffer(
    SDL_GPUDevice* device, const std::vector<uint16_t>& index_data) {
    const uint32_t data_size =
        static_cast<uint32_t>(index_data.size() * sizeof(uint16_t));

    SDL_GPUBufferCreateInfo buf_info = {};
    buf_info.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    buf_info.size                    = data_size;

    SDL_GPUBuffer* ibuf = SDL_CreateGPUBuffer(device, &buf_info);
    if (!ibuf) {
        throw std::runtime_error(
            "graphics.buffer.create_index: SDL_CreateGPUBuffer failed: " +
            std::string(SDL_GetError()));
    }

    SDL_GPUTransferBufferCreateInfo transfer_info = {};
    transfer_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_info.size  = data_size;

    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &transfer_info);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, ibuf);
        throw std::runtime_error(
            "graphics.buffer.create_index: Failed to create transfer "
            "buffer");
    }

    void* mapped = SDL_MapGPUTransferBuffer(device, transfer, false);
    memcpy(mapped, index_data.data(), data_size);
    SDL_UnmapGPUTransferBuffer(device, transfer);

    SDL_GPUCommandBuffer* cmd  = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(cmd);

    SDL_GPUTransferBufferLocation src = {};
    src.transfer_buffer               = transfer;
    src.offset                        = 0;

    SDL_GPUBufferRegion dst = {};
    dst.buffer              = ibuf;
    dst.offset              = 0;
    dst.size                = data_size;

    SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(cmd);

    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return IndexBufferUploadResult{ibuf, data_size};
}

}  // namespace sdl3cpp::services::impl

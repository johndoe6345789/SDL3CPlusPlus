#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_upload.hpp"
#include "services/interfaces/workflow/graphics/graphics_gpu_buffer_transfer.hpp"

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

UploadedGpuBuffers CreateAndUploadGpuBuffers(
    SDL_GPUDevice* device, const std::vector<uint8_t>& vertexBytes,
    const std::vector<uint16_t>& indexValues) {
    const auto vertexSize = static_cast<uint32_t>(vertexBytes.size());
    const auto indexSize =
        static_cast<uint32_t>(indexValues.size() * sizeof(uint16_t));

    const UploadedGpuBuffers buffers =
        CreateGpuVertexIndexBuffers(device, vertexSize, indexSize);
    try {
        UploadGpuBufferData(device, buffers, vertexBytes, indexValues);
    } catch (...) {
        SDL_ReleaseGPUBuffer(device, buffers.vertexBuffer);
        SDL_ReleaseGPUBuffer(device, buffers.indexBuffer);
        throw;
    }
    return buffers;
}

nlohmann::json BuildUploadedMeshMetadata(int vertexCount, int indexCount,
                                         int vertexStride) {
    return json{
        {"vertex_buffer_handle",
         {{"valid", true}, {"vertex_count", vertexCount}}},
        {"index_buffer_handle", {{"valid", true}, {"index_count", indexCount}}},
        {"vertex_layout", {{"stride", vertexStride}}},
    };
}

}  // namespace sdl3cpp::services::impl

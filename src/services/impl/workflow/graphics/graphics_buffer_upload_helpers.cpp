#include "services/interfaces/workflow/graphics/graphics_buffer_upload_helpers.hpp"

#include <cstring>
#include <stdexcept>

using json = nlohmann::json;

namespace sdl3cpp::services::impl {

BufferUploadParams ReadBufferUploadParams(const WorkflowStepDefinition& step) {
    BufferUploadParams params;

    auto it = step.parameters.find("vertex_data_key");
    if (it != step.parameters.end()) {
        params.vertexDataKey = it->second.stringValue;
    }
    it = step.parameters.find("index_data_key");
    if (it != step.parameters.end()) {
        params.indexDataKey = it->second.stringValue;
    }
    it = step.parameters.find("vertex_buffer_key");
    if (it != step.parameters.end()) {
        params.vertexBufferKey = it->second.stringValue;
    }
    it = step.parameters.find("index_buffer_key");
    if (it != step.parameters.end()) {
        params.indexBufferKey = it->second.stringValue;
    }
    it = step.parameters.find("vertex_stride");
    if (it != step.parameters.end()) {
        params.vertexStride = static_cast<int>(it->second.numberValue);
    }

    return params;
}

std::vector<uint8_t> ReadVertexBytesFromContext(const WorkflowContext& context,
                                                const std::string& key) {
    const auto* vertexJson = context.TryGet<json>(key);
    if (!vertexJson || !vertexJson->is_array() || vertexJson->empty()) {
        throw std::runtime_error(
            "graphics.buffer.upload: '" + key +
            "' not found or not a non-empty array in context");
    }

    std::vector<uint8_t> vertexBytes;
    vertexBytes.reserve(vertexJson->size());
    for (const auto& v : *vertexJson) {
        if (!v.is_number()) {
            throw std::runtime_error(
                "graphics.buffer.upload: vertex data must be array of "
                "numbers");
        }
        vertexBytes.push_back(static_cast<uint8_t>(v.get<int>()));
    }
    return vertexBytes;
}

std::vector<uint16_t> ReadIndexValuesFromContext(const WorkflowContext& context,
                                                 const std::string& key) {
    const auto* indexJson = context.TryGet<json>(key);
    if (!indexJson || !indexJson->is_array() || indexJson->empty()) {
        throw std::runtime_error(
            "graphics.buffer.upload: '" + key +
            "' not found or not a non-empty array in context");
    }

    std::vector<uint16_t> indexValues;
    indexValues.reserve(indexJson->size());
    for (const auto& idx : *indexJson) {
        if (!idx.is_number()) {
            throw std::runtime_error(
                "graphics.buffer.upload: index data must be array of "
                "numbers");
        }
        indexValues.push_back(static_cast<uint16_t>(idx.get<int>()));
    }
    return indexValues;
}

UploadedGpuBuffers CreateAndUploadGpuBuffers(
    SDL_GPUDevice* device, const std::vector<uint8_t>& vertexBytes,
    const std::vector<uint16_t>& indexValues) {
    const auto vertexSize = static_cast<uint32_t>(vertexBytes.size());
    const auto indexSize =
        static_cast<uint32_t>(indexValues.size() * sizeof(uint16_t));

    SDL_GPUBufferCreateInfo vbufInfo = {};
    vbufInfo.usage                   = SDL_GPU_BUFFERUSAGE_VERTEX;
    vbufInfo.size                    = vertexSize;
    SDL_GPUBuffer* vbuf              = SDL_CreateGPUBuffer(device, &vbufInfo);
    if (!vbuf) {
        throw std::runtime_error(
            "graphics.buffer.upload: Failed to create vertex buffer: " +
            std::string(SDL_GetError()));
    }

    SDL_GPUBufferCreateInfo ibufInfo = {};
    ibufInfo.usage                   = SDL_GPU_BUFFERUSAGE_INDEX;
    ibufInfo.size                    = indexSize;
    SDL_GPUBuffer* ibuf              = SDL_CreateGPUBuffer(device, &ibufInfo);
    if (!ibuf) {
        SDL_ReleaseGPUBuffer(device, vbuf);
        throw std::runtime_error(
            "graphics.buffer.upload: Failed to create index buffer: " +
            std::string(SDL_GetError()));
    }

    const uint32_t transferSize                  = vertexSize + indexSize;
    SDL_GPUTransferBufferCreateInfo transferInfo = {};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size  = transferSize;
    SDL_GPUTransferBuffer* transfer =
        SDL_CreateGPUTransferBuffer(device, &transferInfo);
    if (!transfer) {
        SDL_ReleaseGPUBuffer(device, vbuf);
        SDL_ReleaseGPUBuffer(device, ibuf);
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
    vDst.buffer                        = vbuf;
    vDst.offset                        = 0;
    vDst.size                          = vertexSize;
    SDL_UploadToGPUBuffer(copyPass, &vSrc, &vDst, false);

    SDL_GPUTransferBufferLocation iSrc = {};
    iSrc.transfer_buffer               = transfer;
    iSrc.offset                        = vertexSize;
    SDL_GPUBufferRegion iDst           = {};
    iDst.buffer                        = ibuf;
    iDst.offset                        = 0;
    iDst.size                          = indexSize;
    SDL_UploadToGPUBuffer(copyPass, &iSrc, &iDst, false);

    SDL_EndGPUCopyPass(copyPass);
    SDL_SubmitGPUCommandBuffer(cmd);
    SDL_ReleaseGPUTransferBuffer(device, transfer);

    return UploadedGpuBuffers{vbuf, ibuf};
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

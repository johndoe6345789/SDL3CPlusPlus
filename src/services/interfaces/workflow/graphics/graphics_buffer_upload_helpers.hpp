#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// graphics.buffer.upload's context-key and layout parameters.
struct BufferUploadParams {
    std::string vertexDataKey   = "vertex_data";
    std::string indexDataKey    = "index_data";
    std::string vertexBufferKey = "gpu_vertex_buffer";
    std::string indexBufferKey  = "gpu_index_buffer";
    int vertexStride            = 16;
};

BufferUploadParams ReadBufferUploadParams(const WorkflowStepDefinition& step);

/// Converts a JSON array of numbers into a flat byte array. Throws
/// std::runtime_error if the array is missing, empty, or non-numeric.
std::vector<uint8_t> ReadVertexBytesFromContext(
    const WorkflowContext& context, const std::string& key);

/// Converts a JSON array of numbers into a uint16 index array. Throws
/// std::runtime_error if the array is missing, empty, or non-numeric.
std::vector<uint16_t> ReadIndexValuesFromContext(
    const WorkflowContext& context, const std::string& key);

/// The pair of GPU buffers created and populated by
/// CreateAndUploadGpuBuffers.
struct UploadedGpuBuffers {
    SDL_GPUBuffer* vertexBuffer = nullptr;
    SDL_GPUBuffer* indexBuffer  = nullptr;
};

/// Creates a vertex buffer and an index buffer on `device`, sized to hold
/// `vertexBytes` and `indexValues`, and uploads both through a single
/// shared transfer buffer. Throws std::runtime_error (and releases any
/// buffer it already created) if any GPU call fails.
UploadedGpuBuffers CreateAndUploadGpuBuffers(
    SDL_GPUDevice* device, const std::vector<uint8_t>& vertexBytes,
    const std::vector<uint16_t>& indexValues);

/// Builds the "cube_mesh" metadata blob describing the uploaded buffers.
nlohmann::json BuildUploadedMeshMetadata(int vertexCount, int indexCount,
                                         int vertexStride);

}  // namespace sdl3cpp::services::impl

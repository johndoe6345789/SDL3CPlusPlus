#pragma once

#include "services/interfaces/workflow_step_definition.hpp"

#include <string>

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

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/graphics/graphics_buffer_upload_params.hpp"

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

}  // namespace sdl3cpp::services::impl

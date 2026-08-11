#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Upload raw vertex and index data from context to GPU buffers.
 *
 * Reads byte arrays (vertex_data, index_data) from context, creates
 * SDL_GPUBuffer objects via a single transfer buffer, and stores the
 * GPU buffer handles back into context. Reusable for any mesh type.
 *
 * Plugin ID: "graphics.buffer.upload"
 *
 * Parameters:
 *   vertex_data_key   (string, default "vertex_data")   - Context key for vertex byte array
 *   index_data_key    (string, default "index_data")     - Context key for index uint16 array
 *   vertex_buffer_key (string, default "gpu_vertex_buffer") - Context key to store vertex GPU buffer
 *   index_buffer_key  (string, default "gpu_index_buffer")  - Context key to store index GPU buffer
 *   vertex_stride     (number, default 16)               - Bytes per vertex (for metadata)
 *
 * Requires in context:
 *   gpu_device (SDL_GPUDevice*) - Initialized GPU device
 *
 * Outputs in context:
 *   {vertex_buffer_key} (SDL_GPUBuffer*) - GPU vertex buffer handle
 *   {index_buffer_key}  (SDL_GPUBuffer*) - GPU index buffer handle
 *   cube_mesh           (json)           - Metadata: counts, stride, validity
 *   geometry_created    (bool)           - Success flag
 */
class WorkflowGraphicsBufferUploadStep final : public IWorkflowStep {
public:
    explicit WorkflowGraphicsBufferUploadStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

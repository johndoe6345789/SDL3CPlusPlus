#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_upload_step.hpp"
#include "services/interfaces/workflow/graphics/graphics_buffer_upload_helpers.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGraphicsBufferUploadStep::WorkflowGraphicsBufferUploadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsBufferUploadStep::GetPluginId() const {
    return "graphics.buffer.upload";
}

void WorkflowGraphicsBufferUploadStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowGraphicsBufferUploadStep", "Execute", "",
                       "Entry");
    }

    const BufferUploadParams params = ReadBufferUploadParams(step);

    try {
        auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        if (!device) {
            throw std::runtime_error(
                "graphics.buffer.upload: GPU device not found in context");
        }

        const std::vector<uint8_t> vertexBytes =
            ReadVertexBytesFromContext(context, params.vertexDataKey);
        const std::vector<uint16_t> indexValues =
            ReadIndexValuesFromContext(context, params.indexDataKey);

        const UploadedGpuBuffers buffers =
            CreateAndUploadGpuBuffers(device, vertexBytes, indexValues);

        context.Set<SDL_GPUBuffer*>(params.vertexBufferKey,
                                    buffers.vertexBuffer);
        context.Set<SDL_GPUBuffer*>(params.indexBufferKey, buffers.indexBuffer);

        const int vertexCount =
            static_cast<int>(vertexBytes.size()) / params.vertexStride;
        const int indexCount = static_cast<int>(indexValues.size());

        context.Set("cube_mesh",
                    BuildUploadedMeshMetadata(vertexCount, indexCount,
                                              params.vertexStride));
        context.Set("geometry_created", true);

        if (logger_) {
            logger_->Info(
                "WorkflowGraphicsBufferUploadStep: Uploaded to GPU (" +
                std::to_string(vertexCount) + " vertices, " +
                std::to_string(indexCount) + " indices, stride=" +
                std::to_string(params.vertexStride) + ")");
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowGraphicsBufferUploadStep::Execute: " +
                           std::string(e.what()));
        }
        context.Set("geometry_created", false);
    }
}

}  // namespace sdl3cpp::services::impl

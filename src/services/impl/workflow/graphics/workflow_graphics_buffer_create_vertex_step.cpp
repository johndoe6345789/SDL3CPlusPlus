#include "services/interfaces/workflow/graphics/workflow_graphics_buffer_create_vertex_step.hpp"
#include "services/interfaces/workflow/graphics/graphics_buffer_create_vertex_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>

#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsBufferCreateVertexStep::WorkflowGraphicsBufferCreateVertexStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsBufferCreateVertexStep::GetPluginId() const {
    return "graphics.buffer.create_vertex";
}

void WorkflowGraphicsBufferCreateVertexStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string verticesKey =
        resolver.GetRequiredInputKey(step, "vertices");
    const std::string outputHandleKey =
        resolver.GetRequiredOutputKey(step, "vertex_handle");

    const auto* vertices_json = context.TryGet<nlohmann::json>(verticesKey);
    if (!vertices_json || !vertices_json->is_array()) {
        throw std::runtime_error(
            "graphics.buffer.create_vertex requires vertices input (array "
            "of floats)");
    }

    const std::vector<float> vertex_data = ParseVertexFloats(*vertices_json);
    if (vertex_data.empty()) {
        throw std::runtime_error(
            "graphics.buffer.create_vertex: vertices array is empty");
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        throw std::runtime_error(
            "graphics.buffer.create_vertex: GPU device not found in "
            "context");
    }

    SDL_GPUBuffer* vbuf = UploadVertexBuffer(device, vertex_data);

    if (logger_) {
        logger_->Trace(
            "WorkflowGraphicsBufferCreateVertexStep", "Execute",
            "vertex_count=" + std::to_string(vertex_data.size() / 3),
            "Vertex buffer created successfully");
    }

    context.Set<SDL_GPUBuffer*>("gpu_vertex_buffer", vbuf);

    const uint32_t data_size =
        static_cast<uint32_t>(vertex_data.size() * sizeof(float));
    nlohmann::json buffer_data = {{"valid", true},
                                  {"vertex_count", vertex_data.size() / 3},
                                  {"size_bytes", data_size}};
    context.Set(outputHandleKey, buffer_data);
}

}  // namespace sdl3cpp::services::impl

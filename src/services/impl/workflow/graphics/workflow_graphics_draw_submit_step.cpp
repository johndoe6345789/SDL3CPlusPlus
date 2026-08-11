#include "services/interfaces/workflow/graphics/workflow_graphics_draw_submit_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsDrawSubmitStep::WorkflowGraphicsDrawSubmitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsDrawSubmitStep::GetPluginId() const {
    return "graphics.draw.submit";
}

void WorkflowGraphicsDrawSubmitStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string programKey = resolver.GetRequiredInputKey(step, "program");
    const std::string vertexHandleKey = resolver.GetRequiredInputKey(step, "vertex_handle");
    const std::string indexHandleKey = resolver.GetRequiredInputKey(step, "index_handle");
    const std::string indexCountKey = resolver.GetRequiredInputKey(step, "index_count");
    const std::string outputDrawCallKey = resolver.GetRequiredOutputKey(step, "draw_call_id");

    // Get GPU objects from context
    SDL_GPURenderPass* render_pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    SDL_GPUGraphicsPipeline* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr);
    SDL_GPUBuffer* vbuf = context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr);
    SDL_GPUBuffer* ibuf = context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);

    if (!render_pass || !pipeline || !vbuf || !ibuf) {
        throw std::runtime_error("graphics.draw.submit: Missing render_pass, pipeline, vertex_buffer, or index_buffer in context");
    }

    // Get index count
    const auto* index_json = context.TryGet<nlohmann::json>(indexHandleKey);
    uint32_t index_count = context.GetInt(indexCountKey, 0);
    if (index_count == 0 && index_json && index_json->contains("index_count")) {
        index_count = (*index_json)["index_count"].get<uint32_t>();
    }
    if (index_count == 0) {
        throw std::runtime_error("graphics.draw.submit: index_count must be > 0");
    }

    // Bind pipeline
    SDL_BindGPUGraphicsPipeline(render_pass, pipeline);

    // Bind vertex buffer
    SDL_GPUBufferBinding vbuf_binding = {};
    vbuf_binding.buffer = vbuf;
    vbuf_binding.offset = 0;
    SDL_BindGPUVertexBuffers(render_pass, 0, &vbuf_binding, 1);

    // Bind index buffer
    SDL_GPUBufferBinding ibuf_binding = {};
    ibuf_binding.buffer = ibuf;
    ibuf_binding.offset = 0;
    SDL_BindGPUIndexBuffer(render_pass, &ibuf_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);

    // Draw indexed primitives
    SDL_DrawGPUIndexedPrimitives(render_pass, index_count, 1, 0, 0, 0);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsDrawSubmitStep", "Execute",
                       "index_count=" + std::to_string(index_count),
                       "Draw call submitted");
    }

    // Store draw call metadata
    static uint32_t draw_call_counter = 0;
    uint32_t draw_call_id = draw_call_counter++;

    nlohmann::json draw_call_data = {
        {"draw_call_id", draw_call_id},
        {"index_count", index_count}
    };
    context.Set(outputDrawCallKey, draw_call_data);
}

}  // namespace sdl3cpp::services::impl

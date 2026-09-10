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

void WorkflowGraphicsDrawSubmitStep::Execute(const WorkflowStepDefinition& step,
                                             WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    // program/vertex_handle inputs are declared but unused below -- see
    // note near the buffer lookups.
    resolver.GetRequiredInputKey(step, "program");
    resolver.GetRequiredInputKey(step, "vertex_handle");
    const std::string indexHandleKey =
        resolver.GetRequiredInputKey(step, "index_handle");
    const std::string indexCountKey =
        resolver.GetRequiredInputKey(step, "index_count");
    const std::string outputDrawCallKey =
        resolver.GetRequiredOutputKey(step, "draw_call_id");

    // GPU objects come from fixed context keys, not the program/
    // vertex_handle inputs above.
    auto* renderPass =
        context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* pipeline =
        context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr);
    auto* vbuf = context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr);
    auto* ibuf = context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr);
    if (!renderPass || !pipeline || !vbuf || !ibuf) {
        throw std::runtime_error(
            "graphics.draw.submit: Missing render_pass, pipeline, "
            "vertex_buffer, or index_buffer in context");
    }

    const auto* indexJson = context.TryGet<nlohmann::json>(indexHandleKey);
    uint32_t indexCount   = context.GetInt(indexCountKey, 0);
    if (indexCount == 0 && indexJson && indexJson->contains("index_count")) {
        indexCount = (*indexJson)["index_count"].get<uint32_t>();
    }
    if (indexCount == 0) {
        throw std::runtime_error(
            "graphics.draw.submit: index_count must be > 0");
    }

    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);

    SDL_GPUBufferBinding vbufBinding = {vbuf, 0};
    SDL_BindGPUVertexBuffers(renderPass, 0, &vbufBinding, 1);

    SDL_GPUBufferBinding ibufBinding = {ibuf, 0};
    SDL_BindGPUIndexBuffer(renderPass, &ibufBinding,
                           SDL_GPU_INDEXELEMENTSIZE_16BIT);

    SDL_DrawGPUIndexedPrimitives(renderPass, indexCount, 1, 0, 0, 0);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsDrawSubmitStep", "Execute",
                       "index_count=" + std::to_string(indexCount),
                       "Draw call submitted");
    }

    static uint32_t drawCallCounter = 0;
    const uint32_t drawCallId       = drawCallCounter++;
    context.Set(outputDrawCallKey, nlohmann::json{{"draw_call_id", drawCallId},
                                                  {"index_count", indexCount}});
}

}  // namespace sdl3cpp::services::impl

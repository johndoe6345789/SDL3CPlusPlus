#include "services/interfaces/workflow/graphics/workflow_graphics_frame_end_step.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsFrameEndStep::WorkflowGraphicsFrameEndStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsFrameEndStep::GetPluginId() const {
    return "graphics.frame.end";
}

void WorkflowGraphicsFrameEndStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string frameIdKey = resolver.GetRequiredInputKey(step, "frame_id");

    const auto* frame_data = context.TryGet<nlohmann::json>(frameIdKey);
    if (!frame_data) {
        throw std::runtime_error("graphics.frame.end requires frame_id input");
    }

    // Check if frame was skipped (minimized window)
    if (frame_data->contains("skipped") && (*frame_data)["skipped"].get<bool>()) {
        return;
    }

    uint32_t frame_id = (*frame_data)["frame_id"].get<uint32_t>();

    SDL_GPURenderPass* render_pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    SDL_GPUCommandBuffer* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_cmd", nullptr);

    if (!render_pass || !cmd) {
        throw std::runtime_error("graphics.frame.end: render pass or command buffer not found in context");
    }

    // End the render pass
    SDL_EndGPURenderPass(render_pass);

    // Submit the command buffer (presents to screen)
    if (!SDL_SubmitGPUCommandBuffer(cmd)) {
        throw std::runtime_error("graphics.frame.end: SDL_SubmitGPUCommandBuffer failed: " +
                                 std::string(SDL_GetError()));
    }

    // Clear transient per-frame state from context
    context.Remove("gpu_render_pass");
    context.Remove("gpu_cmd");
    context.Remove("gpu_swapchain_texture");

    if (logger_) {
        logger_->Trace("WorkflowGraphicsFrameEndStep", "Execute",
                       "frame_id=" + std::to_string(frame_id),
                       "Frame submitted and presented");
    }
}

}  // namespace sdl3cpp::services::impl

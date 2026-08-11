#include "services/interfaces/workflow/rendering/workflow_frame_end_gpu_step.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFrameEndGpuStep::WorkflowFrameEndGpuStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameEndGpuStep::GetPluginId() const {
    return "frame.gpu.end";
}

void WorkflowFrameEndGpuStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);
    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);

    if (pass) {
        SDL_EndGPURenderPass(pass);
        context.Remove("gpu_render_pass");
    }

    if (cmd) {
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Remove("gpu_command_buffer");
    }

    // Increment frame counter
    auto frameNum = context.Get<uint32_t>("frame_number", 0u);
    context.Set<uint32_t>("frame_number", frameNum + 1);
}

}  // namespace sdl3cpp::services::impl

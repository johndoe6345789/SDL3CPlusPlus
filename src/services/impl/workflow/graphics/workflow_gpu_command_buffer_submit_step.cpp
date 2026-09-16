#include "services/interfaces/workflow/graphics/workflow_gpu_command_buffer_submit_step.hpp"
#include "services/interfaces/workflow/graphics/video_recorder_keys.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowGpuCommandBufferSubmitStep::WorkflowGpuCommandBufferSubmitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuCommandBufferSubmitStep::GetPluginId() const {
    return "gpu.command_buffer_submit";
}

void WorkflowGpuCommandBufferSubmitStep::Execute(const WorkflowStepDefinition&,
                                                 WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    if (!cmd) {
        return;
    }

    // video.record.end asks for a fence, to learn when its download is in.
    if (context.GetBool(kGpuSubmitFenceWantedKey, false)) {
        context.Set<bool>(kGpuSubmitFenceWantedKey, false);
        SDL_GPUFence* fence =
            SDL_SubmitGPUCommandBufferAndAcquireFence(cmd);
        context.Set<SDL_GPUFence*>(kGpuSubmitFenceKey, fence);
    } else {
        SDL_SubmitGPUCommandBuffer(cmd);
    }
    context.Remove("gpu_command_buffer");

    if (logger_) {
        logger_->Trace("WorkflowGpuCommandBufferSubmitStep", "Execute", "",
                       "Submitted the frame's GPU command buffer");
    }
}

}  // namespace sdl3cpp::services::impl

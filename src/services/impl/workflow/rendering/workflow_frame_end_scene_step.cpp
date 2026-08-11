#include "services/interfaces/workflow/rendering/workflow_frame_end_scene_step.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowFrameEndSceneStep::WorkflowFrameEndSceneStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameEndSceneStep::GetPluginId() const {
    return "frame.gpu.end_scene";
}

void WorkflowFrameEndSceneStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    auto* pass = context.Get<SDL_GPURenderPass*>("gpu_render_pass", nullptr);

    if (pass) {
        SDL_EndGPURenderPass(pass);
        context.Remove("gpu_render_pass");
    }

    // Keep gpu_command_buffer alive for post-process passes
    // Do NOT submit, do NOT increment frame counter
}

}  // namespace sdl3cpp::services::impl

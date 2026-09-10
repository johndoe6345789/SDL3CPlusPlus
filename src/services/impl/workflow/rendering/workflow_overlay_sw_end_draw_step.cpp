#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_draw_step.hpp"
#include "services/interfaces/workflow/rendering/overlay_sw_end_resources.hpp"

namespace sdl3cpp::services::impl {

WorkflowOverlaySwEndDrawStep::WorkflowOverlaySwEndDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlaySwEndDrawStep::GetPluginId() const {
    return "overlay.sw.end_draw";
}

void WorkflowOverlaySwEndDrawStep::Execute(const WorkflowStepDefinition&,
                                           WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) ||
        !context.GetBool("overlay.ready", false)) {
        return;
    }

    auto* res = context.Get<OverlaySwEndResources*>("overlay_sw_end_resources",
                                                    nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    if (!res || !cmd || !swapchain) {
        return;
    }

    SDL_GPUColorTargetInfo target = {};
    target.texture                = swapchain;
    target.load_op                = SDL_GPU_LOADOP_LOAD;
    target.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) {
        if (logger_) {
            logger_->Warn("overlay.sw.end_draw: BeginGPURenderPass failed");
        }
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, res->pipeline);
    SDL_GPUBufferBinding vb = {res->vertices, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vb, 1);
    SDL_GPUTextureSamplerBinding ts = {res->texture, res->sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &ts, 1);
    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl

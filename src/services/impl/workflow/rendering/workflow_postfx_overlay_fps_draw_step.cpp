#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_draw_step.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxOverlayFpsDrawStep::WorkflowPostfxOverlayFpsDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxOverlayFpsDrawStep::GetPluginId() const {
    return "postfx.overlay_fps_draw";
}

void WorkflowPostfxOverlayFpsDrawStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }
    if (context.GetString(kPostfxCompositeStateKey) !=
        kPostfxCompositeStateDrawn) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("postfx_swapchain_texture", nullptr);
    auto* res = context.Get<GpuTextOverlayResources*>(
        "postfx_overlay_resources", nullptr);
    if (!cmd || !swapchain || !res || !res->pipeline) {
        return;
    }

    // LOADOP_LOAD keeps the composite result that is already in the swapchain.
    SDL_GPUColorTargetInfo target = {};
    target.texture                = swapchain;
    target.load_op                = SDL_GPU_LOADOP_LOAD;
    target.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &target, 1, nullptr);
    if (!pass) {
        if (logger_) {
            logger_->Warn("postfx.overlay_fps_draw: BeginGPURenderPass failed");
        }
        return;
    }

    SDL_BindGPUGraphicsPipeline(pass, res->pipeline);

    SDL_GPUBufferBinding vertices = {res->vertices, 0};
    SDL_BindGPUVertexBuffers(pass, 0, &vertices, 1);

    SDL_GPUTextureSamplerBinding texture = {res->texture, res->sampler};
    SDL_BindGPUFragmentSamplers(pass, 0, &texture, 1);

    SDL_DrawGPUPrimitives(pass, 6, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl

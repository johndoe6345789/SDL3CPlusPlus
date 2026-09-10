#include "services/interfaces/workflow/rendering/workflow_postfx_composite_draw_step.hpp"
#include "services/interfaces/workflow/rendering/postfx_composite_pass.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxCompositeDrawStep::WorkflowPostfxCompositeDrawStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxCompositeDrawStep::GetPluginId() const {
    return "postfx.composite_draw";
}

void WorkflowPostfxCompositeDrawStep::Execute(const WorkflowStepDefinition&,
                                              WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "postfx_composite_pipeline", nullptr);
    auto* hdr = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    auto* sampler =
        context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("postfx_swapchain_texture", nullptr);

    if (!cmd || !pipeline || !hdr || !sampler || !swapchain) {
        context.Set<std::string>(kPostfxCompositeStateKey,
                                 kPostfxCompositeStateMissing);
        if (logger_) {
            logger_->Warn("postfx.composite_draw: Missing required resources");
        }
        return;
    }

    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture                = swapchain;
    colorTarget.load_op                = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) {
        context.Set<std::string>(kPostfxCompositeStateKey,
                                 kPostfxCompositeStateFailed);
        if (logger_) {
            logger_->Warn("postfx.composite_draw: BeginGPURenderPass failed");
        }
        return;
    }

    DrawPostfxCompositeQuad(
        pass, pipeline, hdr, sampler,
        context.Get<SDL_GPUTexture*>("postfx_ssao_texture", nullptr),
        context.Get<SDL_GPUTexture*>("postfx_bloom_result_texture", nullptr));
    SDL_EndGPURenderPass(pass);

    context.Set<std::string>(kPostfxCompositeStateKey,
                             kPostfxCompositeStateDrawn);

    if (logger_) {
        logger_->Trace("WorkflowPostfxCompositeDrawStep", "Execute", "",
                       "Composited HDR target onto the swapchain");
    }
}

}  // namespace sdl3cpp::services::impl

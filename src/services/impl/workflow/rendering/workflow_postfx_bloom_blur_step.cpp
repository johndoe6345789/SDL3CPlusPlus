#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_blur_step.hpp"
#include "services/interfaces/workflow/rendering/postfx_bloom_blur_helpers.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxBloomBlurStep::WorkflowPostfxBloomBlurStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxBloomBlurStep::GetPluginId() const {
    return "postfx.bloom_blur";
}

void WorkflowPostfxBloomBlurStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) return;

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>(
        "postfx_bloom_blur_pipeline", nullptr);
    auto* pingTex =
        context.Get<SDL_GPUTexture*>("postfx_bloom_ping_texture", nullptr);
    auto* pongTex =
        context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture", nullptr);
    auto* sampler =
        context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);

    if (!cmd || !pipeline || !pingTex || !pongTex || !sampler) {
        if (logger_) {
            logger_->Warn(
                "postfx.bloom_blur: Missing required resources, skipping");
        }
        return;
    }

    auto halfW = context.Get<uint32_t>("postfx_bloom_ping_width", 0u);
    auto halfH = context.Get<uint32_t>("postfx_bloom_ping_height", 0u);
    if (halfW == 0 || halfH == 0) return;

    const float texelW = 1.0f / float(halfW);
    const float texelH = 1.0f / float(halfH);

    // Pass 1: Horizontal blur — ping -> pong.
    if (!DrawBloomBlurPass(cmd, pipeline, pingTex, pongTex, sampler, texelW,
                          0.0f)) {
        return;
    }

    // Pass 2: Vertical blur — pong -> ping.
    if (!DrawBloomBlurPass(cmd, pipeline, pongTex, pingTex, sampler, 0.0f,
                          texelH)) {
        return;
    }

    // Result is back in ping texture.
    context.Set<SDL_GPUTexture*>("postfx_bloom_result_texture", pingTex);
}

}  // namespace sdl3cpp::services::impl

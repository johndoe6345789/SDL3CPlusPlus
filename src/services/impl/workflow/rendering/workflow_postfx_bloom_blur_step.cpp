#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_blur_step.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxBloomBlurStep::WorkflowPostfxBloomBlurStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxBloomBlurStep::GetPluginId() const {
    return "postfx.bloom_blur";
}

void WorkflowPostfxBloomBlurStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (context.GetBool("frame_skip", false)) return;

    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("postfx_bloom_blur_pipeline", nullptr);
    auto* pingTex = context.Get<SDL_GPUTexture*>("postfx_bloom_ping_texture", nullptr);
    auto* pongTex = context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture", nullptr);
    auto* sampler = context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);

    if (!cmd || !pipeline || !pingTex || !pongTex || !sampler) {
        if (logger_) logger_->Warn("postfx.bloom_blur: Missing required resources, skipping");
        return;
    }

    auto halfW = context.Get<uint32_t>("postfx_bloom_ping_width", 0u);
    auto halfH = context.Get<uint32_t>("postfx_bloom_ping_height", 0u);
    if (halfW == 0 || halfH == 0) return;

    float texelW = 1.0f / float(halfW);
    float texelH = 1.0f / float(halfH);

    struct { float direction[4]; } uniforms;

    // Pass 1: Horizontal blur — ping → pong
    {
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = pongTex;
        colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
        if (!pass) return;

        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = pingTex;
        binding.sampler = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

        uniforms.direction[0] = texelW;
        uniforms.direction[1] = 0.0f;
        uniforms.direction[2] = 0.0f;
        uniforms.direction[3] = 0.0f;
        SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
    }

    // Pass 2: Vertical blur — pong → ping
    {
        SDL_GPUColorTargetInfo colorTarget = {};
        colorTarget.texture = pingTex;
        colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
        colorTarget.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
        if (!pass) return;

        SDL_BindGPUGraphicsPipeline(pass, pipeline);

        SDL_GPUTextureSamplerBinding binding = {};
        binding.texture = pongTex;
        binding.sampler = sampler;
        SDL_BindGPUFragmentSamplers(pass, 0, &binding, 1);

        uniforms.direction[0] = 0.0f;
        uniforms.direction[1] = texelH;
        uniforms.direction[2] = 0.0f;
        uniforms.direction[3] = 0.0f;
        SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

        SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(pass);
    }

    // Result is back in ping texture
    context.Set<SDL_GPUTexture*>("postfx_bloom_result_texture", pingTex);
}

}  // namespace sdl3cpp::services::impl

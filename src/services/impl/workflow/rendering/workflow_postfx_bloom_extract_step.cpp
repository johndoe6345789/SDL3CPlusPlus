#include "services/interfaces/workflow/rendering/workflow_postfx_bloom_extract_step.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxBloomExtractStep::WorkflowPostfxBloomExtractStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPostfxBloomExtractStep::GetPluginId() const {
    return "postfx.bloom_extract";
}

void WorkflowPostfxBloomExtractStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (context.GetBool("frame_skip", false)) return;

    auto* cmd = context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* pipeline = context.Get<SDL_GPUGraphicsPipeline*>("postfx_bloom_extract_pipeline", nullptr);
    auto* hdrTex = context.Get<SDL_GPUTexture*>("postfx_hdr_texture", nullptr);
    auto* sampler = context.Get<SDL_GPUSampler*>("postfx_linear_sampler", nullptr);

    if (!cmd || !device || !pipeline || !hdrTex || !sampler) {
        if (logger_) logger_->Warn("postfx.bloom_extract: Missing required resources, skipping");
        return;
    }

    auto fw = context.Get<uint32_t>("frame_width", 0u);
    auto fh = context.Get<uint32_t>("frame_height", 0u);
    if (fw == 0 || fh == 0) return;

    uint32_t halfW = fw / 2;
    uint32_t halfH = fh / 2;
    if (halfW == 0) halfW = 1;
    if (halfH == 0) halfH = 1;

    // Create or resize bloom ping/pong textures (half-res RGBA16F)
    auto* pingTex = context.Get<SDL_GPUTexture*>("postfx_bloom_ping_texture", nullptr);
    auto pingW = context.Get<uint32_t>("postfx_bloom_ping_width", 0u);
    auto pingH = context.Get<uint32_t>("postfx_bloom_ping_height", 0u);

    if (!pingTex || pingW != halfW || pingH != halfH) {
        if (pingTex) SDL_ReleaseGPUTexture(device, pingTex);
        auto* oldPong = context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture", nullptr);
        if (oldPong) SDL_ReleaseGPUTexture(device, oldPong);

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        texInfo.width = halfW;
        texInfo.height = halfH;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels = 1;
        texInfo.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

        pingTex = SDL_CreateGPUTexture(device, &texInfo);
        auto* pongTex = SDL_CreateGPUTexture(device, &texInfo);

        context.Set<SDL_GPUTexture*>("postfx_bloom_ping_texture", pingTex);
        context.Set<SDL_GPUTexture*>("postfx_bloom_pong_texture", pongTex);
        context.Set<uint32_t>("postfx_bloom_ping_width", halfW);
        context.Set<uint32_t>("postfx_bloom_ping_height", halfH);
    }

    // Extract bright pixels → ping texture
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = pingTex;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding hdrBinding = {};
    hdrBinding.texture = hdrTex;
    hdrBinding.sampler = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &hdrBinding, 1);

    // Push bloom params: threshold=1.0, soft_knee=0.5
    struct { float params[4]; } uniforms;
    uniforms.params[0] = 1.0f;   // threshold — luminance above this triggers bloom
    uniforms.params[1] = 0.5f;   // soft knee — smooth transition width
    uniforms.params[2] = 0.0f;
    uniforms.params[3] = 0.0f;
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
}

}  // namespace sdl3cpp::services::impl

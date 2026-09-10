#include "services/interfaces/workflow/rendering/bloom_ping_pong.hpp"

namespace sdl3cpp::services::impl {

BloomPingPongTextures EnsureBloomPingPongTextures(SDL_GPUDevice* device,
                                                  WorkflowContext& context,
                                                  uint32_t halfW,
                                                  uint32_t halfH) {
    auto* pingTex =
        context.Get<SDL_GPUTexture*>("postfx_bloom_ping_texture", nullptr);
    auto pingW = context.Get<uint32_t>("postfx_bloom_ping_width", 0u);
    auto pingH = context.Get<uint32_t>("postfx_bloom_ping_height", 0u);

    if (pingTex && pingW == halfW && pingH == halfH) {
        auto* pongTex =
            context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture",
                                        nullptr);
        return {pingTex, pongTex};
    }

    if (pingTex) SDL_ReleaseGPUTexture(device, pingTex);
    auto* oldPong =
        context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture", nullptr);
    if (oldPong) SDL_ReleaseGPUTexture(device, oldPong);

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    texInfo.width = halfW;
    texInfo.height = halfH;
    texInfo.layer_count_or_depth = 1;
    texInfo.num_levels = 1;
    texInfo.usage =
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    pingTex = SDL_CreateGPUTexture(device, &texInfo);
    auto* pongTex = SDL_CreateGPUTexture(device, &texInfo);

    context.Set<SDL_GPUTexture*>("postfx_bloom_ping_texture", pingTex);
    context.Set<SDL_GPUTexture*>("postfx_bloom_pong_texture", pongTex);
    context.Set<uint32_t>("postfx_bloom_ping_width", halfW);
    context.Set<uint32_t>("postfx_bloom_ping_height", halfH);

    return {pingTex, pongTex};
}

bool DrawBloomExtractPass(SDL_GPUCommandBuffer* cmd,
                          SDL_GPUGraphicsPipeline* pipeline,
                          SDL_GPUTexture* hdrTex, SDL_GPUSampler* sampler,
                          SDL_GPUTexture* target) {
    SDL_GPUColorTargetInfo colorTarget = {};
    colorTarget.texture = target;
    colorTarget.load_op = SDL_GPU_LOADOP_DONT_CARE;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* pass =
        SDL_BeginGPURenderPass(cmd, &colorTarget, 1, nullptr);
    if (!pass) return false;

    SDL_BindGPUGraphicsPipeline(pass, pipeline);

    SDL_GPUTextureSamplerBinding hdrBinding = {};
    hdrBinding.texture = hdrTex;
    hdrBinding.sampler = sampler;
    SDL_BindGPUFragmentSamplers(pass, 0, &hdrBinding, 1);

    // Push bloom params: threshold=1.0, soft_knee=0.5
    struct {
        float params[4];
    } uniforms;
    uniforms.params[0] = 1.0f;  // threshold — luminance above this
                                // triggers bloom
    uniforms.params[1] = 0.5f;  // soft knee — smooth transition width
    uniforms.params[2] = 0.0f;
    uniforms.params[3] = 0.0f;
    SDL_PushGPUFragmentUniformData(cmd, 0, &uniforms, sizeof(uniforms));

    SDL_DrawGPUPrimitives(pass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(pass);
    return true;
}

}  // namespace sdl3cpp::services::impl

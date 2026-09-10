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
            context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture", nullptr);
        return {pingTex, pongTex};
    }

    if (pingTex) SDL_ReleaseGPUTexture(device, pingTex);
    auto* oldPong =
        context.Get<SDL_GPUTexture*>("postfx_bloom_pong_texture", nullptr);
    if (oldPong) SDL_ReleaseGPUTexture(device, oldPong);

    SDL_GPUTextureCreateInfo texInfo = {};
    texInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
    texInfo.format                   = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    texInfo.width                    = halfW;
    texInfo.height                   = halfH;
    texInfo.layer_count_or_depth     = 1;
    texInfo.num_levels               = 1;
    texInfo.usage =
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

    pingTex       = SDL_CreateGPUTexture(device, &texInfo);
    auto* pongTex = SDL_CreateGPUTexture(device, &texInfo);

    context.Set<SDL_GPUTexture*>("postfx_bloom_ping_texture", pingTex);
    context.Set<SDL_GPUTexture*>("postfx_bloom_pong_texture", pongTex);
    context.Set<uint32_t>("postfx_bloom_ping_width", halfW);
    context.Set<uint32_t>("postfx_bloom_ping_height", halfH);

    return {pingTex, pongTex};
}

}  // namespace sdl3cpp::services::impl

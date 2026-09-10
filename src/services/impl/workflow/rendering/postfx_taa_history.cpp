#include "services/interfaces/workflow/rendering/postfx_taa_history.hpp"

namespace sdl3cpp::services::impl {

TaaHistoryTextures GetOrCreateTaaHistoryTextures(SDL_GPUDevice* device,
                                                 WorkflowContext& context,
                                                 uint32_t width,
                                                 uint32_t height) {
    auto* historyA  = context.Get<SDL_GPUTexture*>("taa_history_a", nullptr);
    auto* historyB  = context.Get<SDL_GPUTexture*>("taa_history_b", nullptr);
    const auto taaW = context.Get<uint32_t>("taa_width", 0u);
    const auto taaH = context.Get<uint32_t>("taa_height", 0u);

    if (!historyA || !historyB || taaW != width || taaH != height) {
        if (historyA) SDL_ReleaseGPUTexture(device, historyA);
        if (historyB) SDL_ReleaseGPUTexture(device, historyB);

        SDL_GPUTextureCreateInfo texInfo = {};
        texInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
        texInfo.format               = SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
        texInfo.width                = width;
        texInfo.height               = height;
        texInfo.layer_count_or_depth = 1;
        texInfo.num_levels           = 1;
        texInfo.usage =
            SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;

        historyA = SDL_CreateGPUTexture(device, &texInfo);
        historyB = SDL_CreateGPUTexture(device, &texInfo);
        context.Set<SDL_GPUTexture*>("taa_history_a", historyA);
        context.Set<SDL_GPUTexture*>("taa_history_b", historyB);
        context.Set<uint32_t>("taa_width", width);
        context.Set<uint32_t>("taa_height", height);
        context.Set<bool>("taa_ping", true);
    }

    // Ping-pong: read from one history, write to the other.
    const bool ping = context.GetBool("taa_ping", true);
    context.Set<bool>("taa_ping", !ping);
    return ping ? TaaHistoryTextures{historyA, historyB}
                : TaaHistoryTextures{historyB, historyA};
}

}  // namespace sdl3cpp::services::impl

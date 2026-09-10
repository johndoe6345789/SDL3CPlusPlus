#include "services/interfaces/workflow/graphics/gpu_pipeline_formats.hpp"

namespace sdl3cpp::services::impl {

SDL_GPUCullMode ResolveCullMode(const std::string& cullMode) {
    if (cullMode == "front") {
        return SDL_GPU_CULLMODE_FRONT;
    }
    if (cullMode == "none") {
        return SDL_GPU_CULLMODE_NONE;
    }
    return SDL_GPU_CULLMODE_BACK;
}

SDL_GPUTextureFormat ResolveDepthFormat(const std::string& depthFormat) {
    if (depthFormat == "d24_unorm_s8") {
        return SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT;
    }
    return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
}

SDL_GPUTextureFormat ResolveColorTargetFormat(const std::string& colorFormat,
                                              SDL_GPUDevice* device,
                                              SDL_Window* window) {
    if (colorFormat == "rgba16_float") {
        return SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT;
    }
    if (colorFormat == "r8_unorm") {
        return SDL_GPU_TEXTUREFORMAT_R8_UNORM;
    }
    if (colorFormat == "b8g8r8a8_unorm") {
        return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    }
    // "swapchain" (default) — get format from window.
    if (window) {
        return SDL_GetGPUSwapchainTextureFormat(device, window);
    }
    return SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
}

void ApplyAlphaBlendState(SDL_GPUColorTargetDescription& target) {
    target.blend_state.enable_blend          = true;
    target.blend_state.src_color_blendfactor = SDL_GPU_BLENDFACTOR_SRC_ALPHA;
    target.blend_state.dst_color_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
    target.blend_state.color_blend_op        = SDL_GPU_BLENDOP_ADD;
    target.blend_state.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE;
    target.blend_state.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO;
    target.blend_state.alpha_blend_op        = SDL_GPU_BLENDOP_ADD;
}

}  // namespace sdl3cpp::services::impl

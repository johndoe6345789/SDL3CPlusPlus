#include "services/interfaces/workflow/graphics/frame_depth_texture.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

SDL_GPUTexture* GetOrCreateFrameDepthTexture(SDL_GPUDevice* device,
                                             SDL_GPUTexture* existing,
                                             Uint32 width, Uint32 height) {
    if (existing) return existing;

    SDL_GPUTextureCreateInfo depth_info = {};
    depth_info.type                     = SDL_GPU_TEXTURETYPE_2D;
    depth_info.format                   = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    depth_info.width                    = width;
    depth_info.height                   = height;
    depth_info.layer_count_or_depth     = 1;
    depth_info.num_levels               = 1;
    depth_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    SDL_GPUTexture* depth_texture = SDL_CreateGPUTexture(device, &depth_info);
    if (!depth_texture) {
        throw std::runtime_error(
            "graphics.frame.begin: Failed to create depth texture: " +
            std::string(SDL_GetError()));
    }
    return depth_texture;
}

}  // namespace sdl3cpp::services::impl

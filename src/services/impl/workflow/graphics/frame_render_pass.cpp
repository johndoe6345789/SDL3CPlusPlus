#include "services/interfaces/workflow/graphics/frame_render_pass.hpp"

#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

SDL_GPURenderPass* BeginFrameRenderPassOrThrow(SDL_GPUCommandBuffer* cmd,
                                               SDL_GPUTexture* colorTarget,
                                               SDL_GPUTexture* depthTarget,
                                               float r, float g, float b,
                                               float a) {
    SDL_GPUColorTargetInfo color_target = {};
    color_target.texture                = colorTarget;
    color_target.clear_color.r          = r;
    color_target.clear_color.g          = g;
    color_target.clear_color.b          = b;
    color_target.clear_color.a          = a;
    color_target.load_op                = SDL_GPU_LOADOP_CLEAR;
    color_target.store_op               = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depth_target = {};
    depth_target.texture                       = depthTarget;
    depth_target.clear_depth                   = 1.0f;
    depth_target.load_op                       = SDL_GPU_LOADOP_CLEAR;
    depth_target.store_op                      = SDL_GPU_STOREOP_DONT_CARE;

    SDL_GPURenderPass* render_pass =
        SDL_BeginGPURenderPass(cmd, &color_target, 1, &depth_target);
    if (!render_pass) {
        throw std::runtime_error(
            "graphics.frame.begin: SDL_BeginGPURenderPass failed: " +
            std::string(SDL_GetError()));
    }
    return render_pass;
}

}  // namespace sdl3cpp::services::impl

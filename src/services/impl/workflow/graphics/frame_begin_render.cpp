#include "services/interfaces/workflow/graphics/frame_begin_render.hpp"

#include <chrono>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

FrameClearColor ParseFrameClearColorOrThrow(
    const nlohmann::json* clearColorJson) {
    if (!clearColorJson || !clearColorJson->is_array() ||
        clearColorJson->size() != 4) {
        throw std::runtime_error(
            "graphics.frame.begin requires "
            "clear_color input (array of 4 floats [r,g,b,a])");
    }
    return FrameClearColor{
        (*clearColorJson)[0].get<float>(), (*clearColorJson)[1].get<float>(),
        (*clearColorJson)[2].get<float>(), (*clearColorJson)[3].get<float>()};
}

nlohmann::json BuildFrameBeginOutput(uint32_t frameId, bool skipped,
                                     const nlohmann::json& clearColorJson) {
    if (skipped) {
        return nlohmann::json{{"frame_id", frameId}, {"skipped", true}};
    }
    return nlohmann::json{
        {"frame_id", frameId},
        {"clear_color", clearColorJson},
        {"skipped", false},
        {"timestamp",
         static_cast<double>(std::chrono::high_resolution_clock::now()
                                 .time_since_epoch()
                                 .count())}};
}

SDL_GPUCommandBuffer* AcquireFrameCommandBufferOrThrow(SDL_GPUDevice* device) {
    SDL_GPUCommandBuffer* cmd = SDL_AcquireGPUCommandBuffer(device);
    if (!cmd) {
        throw std::runtime_error(
            "graphics.frame.begin: SDL_AcquireGPUCommandBuffer failed: " +
            std::string(SDL_GetError()));
    }
    return cmd;
}

SwapchainAcquireResult AcquireSwapchainTextureOrThrow(SDL_GPUCommandBuffer* cmd,
                                                      SDL_Window* window) {
    SwapchainAcquireResult result{nullptr, 0, 0};
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(cmd, window, &result.texture,
                                               &result.width, &result.height)) {
        SDL_CancelGPUCommandBuffer(cmd);
        throw std::runtime_error(
            "graphics.frame.begin: "
            "SDL_WaitAndAcquireGPUSwapchainTexture failed: " +
            std::string(SDL_GetError()));
    }
    return result;
}

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

std::string DescribeFrameBegin(const FrameClearColor& cc,
                               const SwapchainAcquireResult& swap) {
    return "clear_color=(" + std::to_string(cc.r) + "," + std::to_string(cc.g) +
           "," + std::to_string(cc.b) + "," + std::to_string(cc.a) +
           "), swapchain=" + std::to_string(swap.width) + "x" +
           std::to_string(swap.height);
}

}  // namespace sdl3cpp::services::impl

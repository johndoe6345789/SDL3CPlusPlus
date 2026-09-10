#include "services/interfaces/workflow/rendering/grid_setup_gpu_resources.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace sdl3cpp::services::impl {

void ValidateGridSetupGpuResources(const WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "render.grid.setup: GPU device or window not found in "
            "context");
    }
    if (!context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr)) {
        throw std::runtime_error(
            "render.grid.setup: No GPU pipeline (run "
            "graphics.gpu.shader.load first)");
    }
    if (!context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr) ||
        !context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr)) {
        throw std::runtime_error(
            "render.grid.setup: No vertex/index buffers (run "
            "geometry.create_cube first)");
    }
}

SDL_GPUTexture* CreateGridDepthTexture(SDL_GPUDevice* device, int width,
                                       int height) {
    SDL_GPUTextureCreateInfo depthInfo = {};
    depthInfo.type                     = SDL_GPU_TEXTURETYPE_2D;
    depthInfo.format                   = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    depthInfo.width                    = static_cast<uint32_t>(width);
    depthInfo.height                   = static_cast<uint32_t>(height);
    depthInfo.layer_count_or_depth     = 1;
    depthInfo.num_levels               = 1;
    depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    SDL_GPUTexture* depthTexture = SDL_CreateGPUTexture(device, &depthInfo);
    if (!depthTexture) {
        throw std::runtime_error(
            "render.grid.setup: Failed to create depth texture");
    }
    return depthTexture;
}

}  // namespace sdl3cpp::services::impl

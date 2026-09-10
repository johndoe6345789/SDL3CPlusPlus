#include "services/interfaces/workflow/rendering/render_grid_setup_helpers.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace sdl3cpp::services::impl {

GridSetupParams ReadGridSetupParams(const WorkflowStepDefinition& step) {
    WorkflowStepParameterResolver paramResolver;
    GridSetupParams out;
    auto readParam = [&](const char* name, auto& value) {
        if (const auto* p = paramResolver.FindParameter(step, name)) {
            if (p->type == WorkflowParameterValue::Type::Number) {
                value = static_cast<std::remove_reference_t<decltype(value)>>(
                    p->numberValue);
            }
        }
    };
    readParam("grid_width", out.gridWidth);
    readParam("grid_height", out.gridHeight);
    readParam("grid_spacing", out.gridSpacing);
    readParam("grid_start_x", out.gridStartX);
    readParam("grid_start_y", out.gridStartY);
    readParam("rotation_offset_x", out.rotationOffsetX);
    readParam("rotation_offset_y", out.rotationOffsetY);
    readParam("num_frames", out.numFrames);
    readParam("background_color_r", out.bgColorR);
    readParam("background_color_g", out.bgColorG);
    readParam("background_color_b", out.bgColorB);
    return out;
}

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

nlohmann::json BuildGridConfigJson(const GridSetupParams& params) {
    return nlohmann::json{
        {"grid_width", params.gridWidth},
        {"grid_height", params.gridHeight},
        {"grid_spacing", params.gridSpacing},
        {"grid_start_x", params.gridStartX},
        {"grid_start_y", params.gridStartY},
        {"rotation_offset_x", params.rotationOffsetX},
        {"rotation_offset_y", params.rotationOffsetY},
        {"background_color_r", params.bgColorR},
        {"background_color_g", params.bgColorG},
        {"background_color_b", params.bgColorB},
        {"num_frames", params.numFrames},
    };
}

}  // namespace sdl3cpp::services::impl

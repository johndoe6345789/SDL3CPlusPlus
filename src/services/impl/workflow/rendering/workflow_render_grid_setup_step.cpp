#include "services/interfaces/workflow/rendering/workflow_render_grid_setup_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

WorkflowRenderGridSetupStep::WorkflowRenderGridSetupStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRenderGridSetupStep::GetPluginId() const {
    return "render.grid.setup";
}

void WorkflowRenderGridSetupStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    if (logger_) {
        logger_->Trace("WorkflowRenderGridSetupStep", "Execute", "Entry");
    }

    try {
        WorkflowStepIoResolver ioResolver;
        WorkflowStepParameterResolver paramResolver;

        // Resolve input keys (with safe defaults)
        std::string cameraKey = "camera.state";
        try { cameraKey = ioResolver.GetRequiredInputKey(step, "camera"); } catch (...) {}

        // Validate required GPU resources exist in context
        auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
        if (!device || !window)
            throw std::runtime_error("render.grid.setup: GPU device or window not found in context");
        if (!context.Get<SDL_GPUGraphicsPipeline*>("gpu_pipeline", nullptr))
            throw std::runtime_error("render.grid.setup: No GPU pipeline (run graphics.gpu.shader.load first)");
        if (!context.Get<SDL_GPUBuffer*>("gpu_vertex_buffer", nullptr) ||
            !context.Get<SDL_GPUBuffer*>("gpu_index_buffer", nullptr))
            throw std::runtime_error("render.grid.setup: No vertex/index buffers (run geometry.create_cube first)");

        // Read grid parameters with defaults
        uint32_t gridWidth = 11, gridHeight = 11;
        float gridSpacing = 3.0f;
        float gridStartX = -15.0f, gridStartY = -15.0f;
        float rotationOffsetX = 0.21f, rotationOffsetY = 0.37f;
        float bgColorR = 0.18f, bgColorG = 0.18f, bgColorB = 0.18f;
        uint32_t numFrames = 600;

        auto readParam = [&](const char* name, auto& out) {
            if (const auto* p = paramResolver.FindParameter(step, name)) {
                if (p->type == WorkflowParameterValue::Type::Number) {
                    out = static_cast<std::remove_reference_t<decltype(out)>>(p->numberValue);
                }
            }
        };

        readParam("grid_width", gridWidth);
        readParam("grid_height", gridHeight);
        readParam("grid_spacing", gridSpacing);
        readParam("grid_start_x", gridStartX);
        readParam("grid_start_y", gridStartY);
        readParam("rotation_offset_x", rotationOffsetX);
        readParam("rotation_offset_y", rotationOffsetY);
        readParam("num_frames", numFrames);
        readParam("background_color_r", bgColorR);
        readParam("background_color_g", bgColorG);
        readParam("background_color_b", bgColorB);

        // Create depth texture
        int win_w = 0, win_h = 0;
        SDL_GetWindowSize(window, &win_w, &win_h);

        SDL_GPUTextureCreateInfo depth_info = {};
        depth_info.type = SDL_GPU_TEXTURETYPE_2D;
        depth_info.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
        depth_info.width = static_cast<uint32_t>(win_w);
        depth_info.height = static_cast<uint32_t>(win_h);
        depth_info.layer_count_or_depth = 1;
        depth_info.num_levels = 1;
        depth_info.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

        SDL_GPUTexture* depth_texture = SDL_CreateGPUTexture(device, &depth_info);
        if (!depth_texture) {
            throw std::runtime_error("render.grid.setup: Failed to create depth texture");
        }
        context.Set<SDL_GPUTexture*>("gpu_depth_texture", depth_texture);

        // Store grid configuration as JSON for render.grid.draw
        nlohmann::json gridConfig = {
            {"grid_width",        gridWidth},
            {"grid_height",       gridHeight},
            {"grid_spacing",      gridSpacing},
            {"grid_start_x",      gridStartX},
            {"grid_start_y",      gridStartY},
            {"rotation_offset_x", rotationOffsetX},
            {"rotation_offset_y", rotationOffsetY},
            {"background_color_r", bgColorR},
            {"background_color_g", bgColorG},
            {"background_color_b", bgColorB},
            {"num_frames",        numFrames}
        };
        context.Set<nlohmann::json>("grid.config", std::move(gridConfig));
        context.Set<std::string>("grid.camera_key", cameraKey);

        if (logger_) {
            logger_->Info("WorkflowRenderGridSetupStep: grid=" +
                          std::to_string(gridWidth) + "x" + std::to_string(gridHeight) +
                          ", spacing=" + std::to_string(gridSpacing) +
                          ", frames=" + std::to_string(numFrames) +
                          ", depth=" + std::to_string(win_w) + "x" + std::to_string(win_h));
        }

    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowRenderGridSetupStep::Execute: " + std::string(e.what()));
        }
        context.Set<bool>("render_complete", false);
        context.Set<std::string>("render_error", e.what());
        throw;
    }
}

}  // namespace sdl3cpp::services::impl

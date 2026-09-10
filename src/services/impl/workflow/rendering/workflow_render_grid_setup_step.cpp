#include "services/interfaces/workflow/rendering/workflow_render_grid_setup_step.hpp"
#include "services/interfaces/workflow/rendering/grid_setup_gpu_resources.hpp"
#include "services/interfaces/workflow/rendering/grid_setup_params.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRenderGridSetupStep::WorkflowRenderGridSetupStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRenderGridSetupStep::GetPluginId() const {
    return "render.grid.setup";
}

void WorkflowRenderGridSetupStep::Execute(const WorkflowStepDefinition& step,
                                          WorkflowContext& context) {
    if (logger_) {
        logger_->Trace("WorkflowRenderGridSetupStep", "Execute", "Entry");
    }

    try {
        WorkflowStepIoResolver ioResolver;
        std::string cameraKey = "camera.state";
        try {
            cameraKey = ioResolver.GetRequiredInputKey(step, "camera");
        } catch (...) {}

        ValidateGridSetupGpuResources(context);
        auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
        auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);

        const GridSetupParams params = ReadGridSetupParams(step);

        int winW = 0, winH = 0;
        SDL_GetWindowSize(window, &winW, &winH);
        auto* depthTexture = CreateGridDepthTexture(device, winW, winH);
        context.Set<SDL_GPUTexture*>("gpu_depth_texture", depthTexture);

        context.Set<nlohmann::json>("grid.config", BuildGridConfigJson(params));
        context.Set<std::string>("grid.camera_key", cameraKey);

        if (logger_) {
            logger_->Info("WorkflowRenderGridSetupStep: grid=" +
                          std::to_string(params.gridWidth) + "x" +
                          std::to_string(params.gridHeight) +
                          ", spacing=" + std::to_string(params.gridSpacing) +
                          ", frames=" + std::to_string(params.numFrames) +
                          ", depth=" + std::to_string(winW) + "x" +
                          std::to_string(winH));
        }
    } catch (const std::exception& e) {
        if (logger_) {
            logger_->Error("WorkflowRenderGridSetupStep::Execute: " +
                           std::string(e.what()));
        }
        context.Set<bool>("render_complete", false);
        context.Set<std::string>("render_error", e.what());
        throw;
    }
}

}  // namespace sdl3cpp::services::impl

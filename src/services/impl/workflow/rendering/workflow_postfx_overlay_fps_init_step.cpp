#include "services/interfaces/workflow/rendering/workflow_postfx_overlay_fps_init_step.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <SDL3/SDL_video.h>

namespace sdl3cpp::services::impl {

WorkflowPostfxOverlayFpsInitStep::WorkflowPostfxOverlayFpsInitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowPostfxOverlayFpsInitStep::~WorkflowPostfxOverlayFpsInitStep() {
    DestroyGpuTextOverlayResources(resources_);
}

std::string WorkflowPostfxOverlayFpsInitStep::GetPluginId() const {
    return "postfx.overlay_fps_init";
}

void WorkflowPostfxOverlayFpsInitStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    if (attempted_) {
        // Re-publish so the pointer survives a context reset between frames.
        if (resources_.pipeline) {
            context.Set<GpuTextOverlayResources*>("postfx_overlay_resources",
                                                  &resources_);
        }
        return;
    }

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!device) {
        return;  // No device yet — try again next frame.
    }

    attempted_ = true;

    SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (window) {
        format = SDL_GetGPUSwapchainTextureFormat(device, window);
    }

    const char* failure =
        CreateGpuTextOverlayResources(device, format, resources_);
    if (failure && *failure) {
        if (logger_) {
            logger_->Warn(std::string("postfx.overlay_fps_init: ") + failure +
                          " — FPS display disabled");
        }
        return;
    }

    context.Set<GpuTextOverlayResources*>("postfx_overlay_resources",
                                          &resources_);
    if (logger_) {
        logger_->Info("postfx.overlay_fps_init: FPS overlay initialised");
    }
}

}  // namespace sdl3cpp::services::impl

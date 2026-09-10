#include "services/interfaces/workflow/rendering/workflow_overlay_fps_init_step.hpp"

#include <SDL3/SDL_video.h>

namespace sdl3cpp::services::impl {

WorkflowOverlayFpsInitStep::WorkflowOverlayFpsInitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowOverlayFpsInitStep::~WorkflowOverlayFpsInitStep() {
    DestroyGpuTextOverlayResources(resources_);
}

std::string WorkflowOverlayFpsInitStep::GetPluginId() const {
    return "overlay.fps_init";
}

void WorkflowOverlayFpsInitStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }

    if (!attempted_) {
        if (auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr)) {
            attempted_ = true;

            SDL_GPUTextureFormat format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
            if (auto* window =
                    context.Get<SDL_Window*>("sdl_window", nullptr)) {
                format = SDL_GetGPUSwapchainTextureFormat(device, window);
            }

            const char* failure =
                CreateGpuTextOverlayResources(device, format, resources_);
            if (failure && *failure && logger_) {
                logger_->Warn(std::string("overlay.fps_init: ") + failure +
                              " — FPS display disabled");
            } else if (logger_) {
                logger_->Info("overlay.fps_init: FPS overlay initialised");
            }
        }
    }

    if (resources_.pipeline) {
        context.Set<GpuTextOverlayResources*>("overlay_fps_resources",
                                              &resources_);
    }

    // Exposed for debug.screenshot, which reports overlay readiness.
    context.Set<bool>("overlay_fps_ready", resources_.pipeline != nullptr);
    context.Set<SDL_Surface*>("overlay_fps_surface", resources_.surface);
}

}  // namespace sdl3cpp::services::impl

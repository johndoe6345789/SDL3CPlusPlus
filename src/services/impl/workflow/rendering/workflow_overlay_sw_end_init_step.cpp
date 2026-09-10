#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_init_step.hpp"

#include <SDL3/SDL_render.h>

namespace sdl3cpp::services::impl {

WorkflowOverlaySwEndInitStep::WorkflowOverlaySwEndInitStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

WorkflowOverlaySwEndInitStep::~WorkflowOverlaySwEndInitStep() {
    DestroyOverlaySwEndResources(resources_);
}

std::string WorkflowOverlaySwEndInitStep::GetPluginId() const {
    return "overlay.sw.end_init";
}

void WorkflowOverlaySwEndInitStep::Execute(const WorkflowStepDefinition& step,
                                           WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }
    if (!context.GetBool("overlay.ready", false)) {
        return;
    }

    auto* surface = context.Get<SDL_Surface*>("overlay.surface", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (!surface || !cmd || !swapchain || !device) {
        return;
    }

    if (auto* renderer =
            context.Get<SDL_Renderer*>("overlay.renderer", nullptr)) {
        SDL_RenderPresent(renderer);
    }

    if (!attempted_) {
        attempted_ = true;
        std::string vertPath, fragPath;
        ResolveOverlaySwEndShaderPaths(step, device, vertPath, fragPath);
        auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
        const bool created =
            CreateOverlaySwEndResources(device, window, surface->w, surface->h,
                                        vertPath, fragPath, resources_);
        if (!created && logger_) {
            logger_->Warn(
                "overlay.sw.end_init: GPU overlay init failed — "
                "SW overlay blit disabled");
        } else if (logger_) {
            logger_->Info("overlay.sw.end_init: overlay blit ready");
        }
    }

    if (resources_.pipeline) {
        context.Set<OverlaySwEndResources*>("overlay_sw_end_resources",
                                            &resources_);
    }
}

}  // namespace sdl3cpp::services::impl

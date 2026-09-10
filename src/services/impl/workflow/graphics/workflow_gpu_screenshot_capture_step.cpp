#include "services/interfaces/workflow/graphics/workflow_gpu_screenshot_capture_step.hpp"
#include "services/interfaces/workflow/graphics/gpu_swapchain_capture.hpp"
#include "services/interfaces/workflow/graphics/graphics_screenshot_request_helpers.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <SDL3/SDL_gpu.h>

#include <string>

namespace sdl3cpp::services::impl {

WorkflowGpuScreenshotCaptureStep::WorkflowGpuScreenshotCaptureStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuScreenshotCaptureStep::GetPluginId() const {
    return "gpu.screenshot_capture";
}

void WorkflowGpuScreenshotCaptureStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    const auto* path = context.TryGet<std::string>("gpu_screenshot_output_path");
    if (!path || path->empty()) {
        // No screenshot requested this frame — the common case, stay quiet.
        return;
    }
    if (context.GetBool("frame_skip", false)) {
        if (logger_) {
            logger_->Warn("gpu.screenshot_capture: frame skipped, "
                         "dropping requested screenshot");
        }
        return;
    }
    if (context.GetString(kPostfxCompositeStateKey) !=
        kPostfxCompositeStateMissing) {
        if (logger_) {
            logger_->Warn(
                "gpu.screenshot_capture: postfx composited this frame ("
                "state='" +
                context.GetString(kPostfxCompositeStateKey) +
                "'), this fallback capture only runs when postfx resources "
                "are missing");
        }
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    if (!cmd || !device || !window || !swapchain) {
        if (logger_) {
            logger_->Warn(
                "gpu.screenshot_capture: missing cmd=" +
                std::to_string(cmd != nullptr) +
                " device=" + std::to_string(device != nullptr) +
                " window=" + std::to_string(window != nullptr) +
                " swapchain=" + std::to_string(swapchain != nullptr));
        }
        return;
    }

    CaptureGpuSwapchainToBmp(
        cmd, device, swapchain, context.Get<uint32_t>("frame_width", 1),
        context.Get<uint32_t>("frame_height", 1),
        SDL_GetGPUSwapchainTextureFormat(device, window),
        ResolveScreenshotOutputPath(*path), logger_);

    context.Set<std::string>("gpu_screenshot_output_path", std::string(""));
    context.Remove("gpu_command_buffer");
}

}  // namespace sdl3cpp::services::impl

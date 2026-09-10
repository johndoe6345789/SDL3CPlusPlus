#include "services/interfaces/workflow/graphics/workflow_gpu_screenshot_capture_step.hpp"
#include "services/interfaces/workflow/graphics/gpu_swapchain_capture.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"

#include <SDL3/SDL_gpu.h>

namespace sdl3cpp::services::impl {

WorkflowGpuScreenshotCaptureStep::WorkflowGpuScreenshotCaptureStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGpuScreenshotCaptureStep::GetPluginId() const {
    return "gpu.screenshot_capture";
}

void WorkflowGpuScreenshotCaptureStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    if (context.GetBool("frame_skip", false)) {
        return;
    }
    if (context.GetString(kPostfxCompositeStateKey) !=
        kPostfxCompositeStateMissing) {
        return;
    }

    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    const auto* path = context.TryGet<std::string>("screenshot_output_path");
    if (!cmd || !device || !swapchain || !path || path->empty()) {
        return;
    }

    CaptureGpuSwapchainToBmp(
        cmd, device, swapchain, context.Get<uint32_t>("frame_width", 1),
        context.Get<uint32_t>("frame_height", 1), *path, logger_);

    context.Set<std::string>("screenshot_output_path", std::string(""));
    context.Remove("gpu_command_buffer");
}

}  // namespace sdl3cpp::services::impl

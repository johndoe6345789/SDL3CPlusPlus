#include "services/interfaces/workflow/graphics/workflow_graphics_screenshot_request_step.hpp"
#include "services/interfaces/workflow/graphics/graphics_screenshot_request_helpers.hpp"
#include "services/interfaces/workflow/graphics/gpu_swapchain_capture.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsScreenshotRequestStep::WorkflowGraphicsScreenshotRequestStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsScreenshotRequestStep::GetPluginId() const {
    return "graphics.screenshot.request";
}

void WorkflowGraphicsScreenshotRequestStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string outputPathKey =
        resolver.GetRequiredInputKey(step, "output_path");
    const std::string outputSuccessKey =
        resolver.GetRequiredOutputKey(step, "success");

    const auto* outputPath = context.TryGet<std::string>(outputPathKey);
    if (!outputPath || outputPath->empty()) {
        // No path set yet — skip silently (e.g. waiting for frame
        // threshold).
        context.Set(outputSuccessKey, false);
        return;
    }

    const std::string resolvedPath =
        ResolveScreenshotOutputPath(*outputPath);

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "graphics.screenshot.request: GPU device or window not found");
    }

    const StagingCapture capture =
        BlitSwapchainToStagingTexture(device, window);
    if (!capture.texture) {
        context.Set(outputSuccessKey, false);
        return;
    }

    SDL_GPUCommandBuffer* dlCmd = SDL_AcquireGPUCommandBuffer(device);
    const bool saved = CaptureGpuSwapchainToBmp(
        dlCmd, device, capture.texture, capture.width, capture.height,
        ToBmpPath(resolvedPath), logger_);
    SDL_ReleaseGPUTexture(device, capture.texture);

    context.Set(outputSuccessKey, saved);
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/graphics/workflow_graphics_framebuffer_readback_step.hpp"
#include "services/interfaces/workflow/graphics/gpu_framebuffer_readback.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowGraphicsFramebufferReadbackStep::
    WorkflowGraphicsFramebufferReadbackStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsFramebufferReadbackStep::GetPluginId() const {
    return "graphics.framebuffer.readback";
}

void WorkflowGraphicsFramebufferReadbackStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string srcTexKeyKey =
        resolver.GetRequiredInputKey(step, "source_texture_key");
    const std::string outputDataKey =
        resolver.GetRequiredOutputKey(step, "output_key");
    const std::string outputWidthKey =
        resolver.GetRequiredOutputKey(step, "output_width");
    const std::string outputHeightKey =
        resolver.GetRequiredOutputKey(step, "output_height");
    const std::string outputSuccessKey =
        resolver.GetRequiredOutputKey(step, "success");
    // Context key name that holds the source texture.
    const auto* srcTexKeyPtr = context.TryGet<std::string>(srcTexKeyKey);
    const std::string srcTexKey = (srcTexKeyPtr && !srcTexKeyPtr->empty())
                                      ? *srcTexKeyPtr
                                      : std::string("gpu_swapchain_texture");

    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "graphics.framebuffer.readback: GPU device/window missing");
    }
    // srcTexKey is only validated here; the pixels always come from the
    // swapchain via BlitSwapchainToStaging, matching the original.
    if (!context.Get<SDL_GPUTexture*>(srcTexKey, nullptr)) {
        throw std::runtime_error(
            "graphics.framebuffer.readback: source texture '" + srcTexKey +
            "' not found in context");
    }
    int win_w = 0, win_h = 0;
    SDL_GetWindowSize(window, &win_w, &win_h);
    const auto fail = [&] { context.Set(outputSuccessKey, false); };
    if (win_w <= 0 || win_h <= 0) { fail(); return; }

    const BlittedSwapchainStaging staging =
        BlitSwapchainToStaging(device, window);
    if (!staging.texture) { fail(); return; }

    std::vector<uint8_t> pixel_data =
        DownloadStagingTexture(device, staging);
    if (pixel_data.empty()) { fail(); return; }

    context.Set(outputDataKey, std::move(pixel_data));
    context.Set(outputWidthKey, staging.width);
    context.Set(outputHeightKey, staging.height);
    context.Set(outputSuccessKey, true);
    if (logger_) {
        logger_->Info("graphics.framebuffer.readback: Read back " +
                      std::to_string(staging.width) + "x" +
                      std::to_string(staging.height) + " into '" +
                      outputDataKey + "'");
    }
}

}  // namespace sdl3cpp::services::impl

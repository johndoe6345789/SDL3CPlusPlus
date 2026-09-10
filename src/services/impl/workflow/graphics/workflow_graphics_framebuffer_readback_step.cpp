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
    const FramebufferReadbackKeys keys =
        ResolveFramebufferReadbackKeys(resolver, step);
    // Context key name that holds the source texture.
    const auto* srcTexKeyPtr =
        context.TryGet<std::string>(keys.sourceTextureKeyKey);
    const std::string srcTexKey = (srcTexKeyPtr && !srcTexKeyPtr->empty())
                                      ? *srcTexKeyPtr
                                      : "gpu_swapchain_texture";
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
    const auto fail = [&] { context.Set(keys.outputSuccessKey, false); };
    if (!HasValidWindowSize(window)) {
        fail();
        return;
    }

    const BlittedSwapchainStaging staging =
        BlitSwapchainToStaging(device, window);
    if (!staging.texture) {
        fail();
        return;
    }

    std::vector<uint8_t> pixel_data = DownloadStagingTexture(device, staging);
    if (pixel_data.empty()) {
        fail();
        return;
    }

    context.Set(keys.outputDataKey, std::move(pixel_data));
    context.Set(keys.outputWidthKey, staging.width);
    context.Set(keys.outputHeightKey, staging.height);
    context.Set(keys.outputSuccessKey, true);
    if (logger_) {
        logger_->Info("graphics.framebuffer.readback: Read back " +
                      std::to_string(staging.width) + "x" +
                      std::to_string(staging.height) + " into '" +
                      keys.outputDataKey + "'");
    }
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/rendering/workflow_frame_begin_gpu_step.hpp"
#include "services/interfaces/workflow/rendering/frame_begin_shared_helpers.hpp"
#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowFrameBeginGpuStep::WorkflowFrameBeginGpuStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameBeginGpuStep::GetPluginId() const {
    return "frame.gpu.begin";
}

void WorkflowFrameBeginGpuStep::Execute(const WorkflowStepDefinition& step,
                                        WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "frame.gpu.begin: No GPU device or window in context");
    }

    const ClearColorParams clear = ReadClearColorParams(step);

    const AcquiredSwapchain swap = AcquireSwapchainForFrame(device, window);
    if (!swap.ok) {
        context.Set<bool>("frame_skip", true);
        return;
    }

    auto* depthTex = GetOrResizeTexture(
        device, context, "gpu_depth_texture", "gpu_depth_width",
        "gpu_depth_height", SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET, swap.width, swap.height);

    SDL_GPURenderPass* pass =
        BeginColorDepthRenderPass(swap.cmd, swap.texture, clear, depthTex);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(swap.cmd);
        context.Set<bool>("frame_skip", true);
        return;
    }

    // Store for subsequent steps.
    context.Set<SDL_GPUCommandBuffer*>("gpu_command_buffer", swap.cmd);
    context.Set<SDL_GPURenderPass*>("gpu_render_pass", pass);
    context.Set<SDL_GPUTexture*>("gpu_swapchain_texture", swap.texture);
    context.Set<bool>("frame_skip", false);
    context.Set<uint32_t>("frame_width", swap.width);
    context.Set<uint32_t>("frame_height", swap.height);
}

}  // namespace sdl3cpp::services::impl

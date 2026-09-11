#include "services/interfaces/workflow/rendering/workflow_frame_begin_offscreen_step.hpp"
#include "services/interfaces/workflow/rendering/frame_begin_shared_helpers.hpp"
#include "services/interfaces/workflow/rendering/frame_render_scale.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>

#include <algorithm>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowFrameBeginOffscreenStep::WorkflowFrameBeginOffscreenStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowFrameBeginOffscreenStep::GetPluginId() const {
    return "frame.gpu.begin_offscreen";
}

void WorkflowFrameBeginOffscreenStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "frame.gpu.begin_offscreen: No GPU device or window in "
            "context");
    }

    const ClearColorParams clear = ReadClearColorParams(step);

    const AcquiredSwapchain swap = AcquireSwapchainForFrame(device, window);
    if (!swap.ok) {
        context.Set<bool>("frame_skip", true);
        return;
    }

    context.Set<SDL_GPUTexture*>("postfx_swapchain_texture", swap.texture);

    // render_scale above 1 supersamples; frame_width/height stay the
    // window's, render_width/height are the scene target's.
    const float scale = ReadFrameRenderScale(step);
    const uint32_t renderWidth =
        std::max(1u, static_cast<uint32_t>(swap.width * scale));
    const uint32_t renderHeight =
        std::max(1u, static_cast<uint32_t>(swap.height * scale));

    auto* hdrTex = GetOrResizeTexture(
        device, context, "postfx_hdr_texture", "postfx_hdr_width",
        "postfx_hdr_height", SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
        SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        renderWidth, renderHeight);

    auto* depthTex = GetOrResizeTexture(
        device, context, "gpu_depth_texture", "gpu_depth_width",
        "gpu_depth_height", SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
        SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET |
            SDL_GPU_TEXTUREUSAGE_SAMPLER,
        renderWidth, renderHeight);

    SDL_GPURenderPass* pass =
        BeginColorDepthRenderPass(swap.cmd, hdrTex, clear, depthTex);
    if (!pass) {
        SDL_SubmitGPUCommandBuffer(swap.cmd);
        context.Set<bool>("frame_skip", true);
        return;
    }

    // Store for subsequent draw steps.
    context.Set<SDL_GPUCommandBuffer*>("gpu_command_buffer", swap.cmd);
    context.Set<SDL_GPURenderPass*>("gpu_render_pass", pass);
    context.Set<bool>("frame_skip", false);
    context.Set<uint32_t>("frame_width", swap.width);
    context.Set<uint32_t>("frame_height", swap.height);
    context.Set<uint32_t>("render_width", renderWidth);
    context.Set<uint32_t>("render_height", renderHeight);
}

}  // namespace sdl3cpp::services::impl

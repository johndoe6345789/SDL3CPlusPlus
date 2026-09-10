#include "services/interfaces/workflow/graphics/workflow_graphics_frame_begin_step.hpp"
#include "services/interfaces/workflow/graphics/frame_begin_describe.hpp"
#include "services/interfaces/workflow/graphics/frame_clear_color.hpp"
#include "services/interfaces/workflow/graphics/frame_depth_texture.hpp"
#include "services/interfaces/workflow/graphics/frame_render_pass.hpp"
#include "services/interfaces/workflow/graphics/frame_swapchain.hpp"
#include "services/interfaces/workflow/workflow_step_io_resolver.hpp"

#include <SDL3/SDL_gpu.h>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace sdl3cpp::services::impl {

WorkflowGraphicsFrameBeginStep::WorkflowGraphicsFrameBeginStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowGraphicsFrameBeginStep::GetPluginId() const {
    return "graphics.frame.begin";
}

void WorkflowGraphicsFrameBeginStep::Execute(const WorkflowStepDefinition& step,
                                             WorkflowContext& context) {
    WorkflowStepIoResolver resolver;
    const std::string clearColorKey =
        resolver.GetRequiredInputKey(step, "clear_color");
    const std::string outputFrameKey =
        resolver.GetRequiredOutputKey(step, "frame_id");

    const auto* clear_color_json =
        context.TryGet<nlohmann::json>(clearColorKey);
    const FrameClearColor cc = ParseFrameClearColorOrThrow(clear_color_json);

    SDL_GPUDevice* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    SDL_Window* window    = context.Get<SDL_Window*>("sdl_window", nullptr);
    if (!device || !window) {
        throw std::runtime_error(
            "graphics.frame.begin: GPU device or "
            "SDL window not found in context");
    }

    SDL_GPUCommandBuffer* cmd = AcquireFrameCommandBufferOrThrow(device);
    const SwapchainAcquireResult swap =
        AcquireSwapchainTextureOrThrow(cmd, window);

    static uint32_t frame_counter = 0;

    if (!swap.texture) {
        // Window minimized or not visible - submit empty command buffer
        SDL_SubmitGPUCommandBuffer(cmd);
        context.Set(outputFrameKey, BuildFrameBeginOutput(frame_counter++, true,
                                                          *clear_color_json));
        return;
    }

    // Store command buffer and swapchain texture for render pass and
    // frame end
    context.Set<SDL_GPUCommandBuffer*>("gpu_cmd", cmd);
    context.Set<SDL_GPUTexture*>("gpu_swapchain_texture", swap.texture);

    SDL_GPUTexture* depth_texture =
        context.Get<SDL_GPUTexture*>("gpu_depth_texture", nullptr);
    depth_texture = GetOrCreateFrameDepthTexture(device, depth_texture,
                                                 swap.width, swap.height);
    context.Set<SDL_GPUTexture*>("gpu_depth_texture", depth_texture);

    SDL_GPURenderPass* render_pass = BeginFrameRenderPassOrThrow(
        cmd, swap.texture, depth_texture, cc.r, cc.g, cc.b, cc.a);
    context.Set<SDL_GPURenderPass*>("gpu_render_pass", render_pass);

    if (logger_) {
        logger_->Trace("WorkflowGraphicsFrameBeginStep", "Execute",
                       DescribeFrameBegin(cc, swap),
                       "Frame begin: render pass started");
    }

    context.Set(outputFrameKey, BuildFrameBeginOutput(frame_counter++, false,
                                                      *clear_color_json));
}

}  // namespace sdl3cpp::services::impl

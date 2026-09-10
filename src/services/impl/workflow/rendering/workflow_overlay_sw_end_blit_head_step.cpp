#include "services/interfaces/workflow/rendering/workflow_overlay_sw_end_blit_head_step.hpp"
#include "services/interfaces/workflow/rendering/overlay_head_portrait.hpp"

namespace sdl3cpp::services::impl {

WorkflowOverlaySwEndBlitHeadStep::WorkflowOverlaySwEndBlitHeadStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowOverlaySwEndBlitHeadStep::GetPluginId() const {
    return "overlay.sw.end_blit_head";
}

void WorkflowOverlaySwEndBlitHeadStep::Execute(const WorkflowStepDefinition&,
                                               WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) ||
        !context.GetBool("overlay.ready", false)) {
        return;
    }

    auto* headTex =
        context.Get<SDL_GPUTexture*>("overlay.head_gpu_tex", nullptr);
    if (!headTex) {
        return;
    }

    auto* res = context.Get<OverlaySwEndResources*>("overlay_sw_end_resources",
                                                    nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    if (!res || !cmd || !swapchain) {
        return;
    }

    // Fixed overlay resolution the face rect (from q3.hud_head_render) is
    // expressed in, independent of the actual SW surface size.
    constexpr int kOverlayWidth  = 640;
    constexpr int kOverlayHeight = 360;

    BlitHeadPortrait(cmd, swapchain, *res, headTex,
                     context.Get<float>("hud.face_rect_x", 354.0f),
                     context.Get<float>("hud.face_rect_y", 322.0f),
                     context.Get<float>("hud.face_rect_w", 32.0f),
                     context.Get<float>("hud.face_rect_h", 32.0f),
                     kOverlayWidth, kOverlayHeight);
}

}  // namespace sdl3cpp::services::impl

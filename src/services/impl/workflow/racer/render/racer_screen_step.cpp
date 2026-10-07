#include "services/interfaces/workflow/racer/render/racer_screen_step.hpp"

#include "services/interfaces/workflow/racer/flow/racer_flow_text.hpp"
#include "services/interfaces/workflow/racer/racer_step_params.hpp"
#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
void WorkflowRacerScreenStep::Execute(const WorkflowStepDefinition& step,
                                      WorkflowContext& context) {
    const RacerFlow& flow = state_->flow;
    if (context.GetBool("frame_skip", false) || !flow.initialised ||
        flow.phase == RacerPhase::Racing) {
        return;
    }
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (!cmd || !swapchain || !res || !res->pipeline) return;
    if (!prepared_) {
        const float w = static_cast<float>(
            context.Get<uint32_t>("frame_width", 1920u));
        const float h = static_cast<float>(
            context.Get<uint32_t>("frame_height", 1080u));
        Prepare(step, res->device, cmd, w / h);
    }
    const RacerScreenContent screen = Content();
    std::string key = std::to_string(static_cast<int>(flow.phase));
    for (const auto& line : screen.lines) key += line.text + "|";
    if (key != shown_) {
        UploadRacerPanelText(text_, cmd, screen.lines, screen.shade);
        shown_ = key;
    }
    if (screen.background) {
        DrawRacerPanel(*screen.background, cmd, swapchain, res->pipeline,
                       res->sampler);
    }
    DrawRacerPanel(text_, cmd, swapchain, res->pipeline, res->sampler);
}

}  // namespace sdl3cpp::services::impl

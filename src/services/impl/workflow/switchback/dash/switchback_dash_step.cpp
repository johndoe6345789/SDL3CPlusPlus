#include "services/interfaces/workflow/switchback/dash/switchback_dash_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"
#include "services/interfaces/workflow/switchback/dash/switchback_dash_frame.hpp"
#include "services/interfaces/workflow/switchback/dash/switchback_menu_frame.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_gpu.h>

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowSwitchbackDashStep::WorkflowSwitchbackDashStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state,
    std::shared_ptr<SwitchbackSession> session)
    : logger_(std::move(logger)),
      state_(std::move(state)),
      session_(std::move(session)) {}

std::string WorkflowSwitchbackDashStep::GetPluginId() const {
    return "switchback.dash";
}

void WorkflowSwitchbackDashStep::Execute(const WorkflowStepDefinition& step,
                                         WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false) ||
        context.GetString(kPostfxCompositeStateKey) !=
            kPostfxCompositeStateDrawn) {
        return;
    }
    auto* device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    auto* window = context.Get<SDL_Window*>("sdl_window", nullptr);
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("postfx_swapchain_texture", nullptr);
    if (!device || !window || !cmd || !swapchain) return;
    if (!hud_.tried) {
        LoadGta5Hud(hud_, device,
                    SDL_GetGPUSwapchainTextureFormat(device, window),
                    Gta5ParameterOr(step, "minimap_dir", ""),
                    state_->uploads, logger_);
    }
    if (!hud_.ready) return;

    const auto width = static_cast<int>(
        context.Get<uint32_t>("frame_width", 1280u));
    const auto height = static_cast<int>(
        context.Get<uint32_t>("frame_height", 960u));
    if (session_->screen == SwitchbackScreen::Menu) {
        const Gta5MapFrame menu =
            BuildSwitchbackMenuFrame(hud_, width, height, *session_);
        DrawGta5MapOverlay(hud_.overlay, device, cmd, swapchain, menu);
        return;
    }
    if (!session_->showDash) return;

    const Gta5HudState s = ReadGta5HudState(context, *state_);
    SwitchbackRaceProgress race;
    race.onRoute = context.GetBool("switchback.route.active", false);
    race.passed = context.Get<int>("switchback.checkpoint.passed", 0);
    race.total = context.Get<int>("switchback.checkpoint.total", 0);
    race.finished = context.GetBool("switchback.race.finished", false);
    const Gta5MapFrame frame =
        BuildSwitchbackDashFrame(hud_, width, height, s, race);
    DrawGta5MapOverlay(hud_.overlay, device, cmd, swapchain, frame);
}

}  // namespace sdl3cpp::services::impl

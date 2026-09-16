#include "services/interfaces/workflow/gta5/hud/gta5_hud_step.hpp"

#include "services/interfaces/workflow/gta5/core/gta5_step_params.hpp"
#include "services/interfaces/workflow/gta5/hud/gta5_map_build.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>

#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5HudStep::WorkflowGta5HudStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5HudStep::GetPluginId() const { return "gta5.hud"; }

void WorkflowGta5HudStep::Execute(const WorkflowStepDefinition& step,
                                  WorkflowContext& context) {
    if (!state_ || context.GetBool("frame_skip", false) ||
        context.GetBool("gta5.map.open", false) ||
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

    Gta5HudState s = ReadGta5HudState(context, *state_);
    s.showWeapon = Gta5HudWeaponShown(weaponTimer_, s, SDL_GetTicks());
    const Gta5MapFrame frame = BuildGta5HudFrame(
        hud_, static_cast<int>(context.Get<uint32_t>("frame_width", 1280u)),
        static_cast<int>(context.Get<uint32_t>("frame_height", 960u)), s);
    DrawGta5MapOverlay(hud_.overlay, device, cmd, swapchain, frame);
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/gta5/gta5_hud_step.hpp"

#include "services/interfaces/workflow/gta5/gta5_map_build.hpp"
#include "services/interfaces/workflow/rendering/workflow_postfx_composite_state.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowGta5HudStep::WorkflowGta5HudStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<Gta5StreamState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

std::string WorkflowGta5HudStep::GetPluginId() const { return "gta5.hud"; }

void WorkflowGta5HudStep::Execute(const WorkflowStepDefinition&,
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
                    state_->uploads, logger_);
    }
    if (!hud_.ready) return;

    Gta5HudState s;
    s.health = context.Get<float>("gta5.player.health", 100.f);
    s.armour = context.Get<float>("gta5.player.armour", 0.f);
    s.weapon = context.GetString("gta5.weapon.name", "");
    s.clip = context.Get<int>("gta5.weapon.clip", -1);
    s.reserve = context.Get<int>("gta5.weapon.reserve", 0);
    s.driving = state_->seated >= 0;
    s.kmh = std::abs(context.Get<float>("gta5.car.speed", 0.f)) * 3.6f;
    s.revs = context.Get<float>("gta5.car.revs", 0.f);
    s.gear = context.Get<int>("gta5.car.gear", 1);
    s.prompt = context.GetString("gta5.prompt", "");
    s.menuOpen = context.GetBool("gta5.menu.open", false);
    if (s.menuOpen) s.menu = context.Get<Gta5Menu>("gta5.menu", Gta5Menu{});
    s.wheel.open = context.GetBool("gta5.wheel.open", false);
    if (s.wheel.open) {
        s.wheel.items = context.Get<std::vector<std::string>>(
            "gta5.wheel.items", {});
        s.wheel.selected = context.Get<int>("gta5.wheel.selected", 0);
    }
    const Gta5MapFrame frame = BuildGta5HudFrame(
        hud_, static_cast<int>(context.Get<uint32_t>("frame_width", 1280u)),
        static_cast<int>(context.Get<uint32_t>("frame_height", 960u)), s);
    DrawGta5MapOverlay(hud_.overlay, device, cmd, swapchain, frame);
}

}  // namespace sdl3cpp::services::impl

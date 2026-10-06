#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <utility>

namespace sdl3cpp::services::impl {

WorkflowRacerHudStep::WorkflowRacerHudStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

WorkflowRacerHudStep::~WorkflowRacerHudStep() {
    DestroyRacerPanel(hud_);
    DestroyRacerPanel(banner_);
}

std::string WorkflowRacerHudStep::GetPluginId() const {
    return "racer.hud.text";
}

void WorkflowRacerHudStep::Execute(const WorkflowStepDefinition&,
                                   WorkflowContext& context) {
    if (context.GetBool("frame_skip", false) || !state_->loaded ||
        state_->flow.phase != RacerPhase::Racing) {
        return;
    }
    auto* cmd =
        context.Get<SDL_GPUCommandBuffer*>("gpu_command_buffer", nullptr);
    auto* swapchain =
        context.Get<SDL_GPUTexture*>("gpu_swapchain_texture", nullptr);
    auto* res =
        context.Get<GpuTextOverlayResources*>("overlay_fps_resources", nullptr);
    if (!cmd || !swapchain || !res || !res->pipeline) return;
    const float fw =
        static_cast<float>(context.Get<uint32_t>("frame_width", 1920u));
    const float fh =
        static_cast<float>(context.Get<uint32_t>("frame_height", 1080u));
    if (!hud_.texture) {
        if (!CreateRacerTextPanel(res->device, kHudWidth, kHudHeight, hud_) ||
            !CreateRacerTextPanel(res->device, kBannerWidth, kBannerHeight,
                                  banner_)) {
            return;
        }
        UploadRacerPanelRect(hud_, cmd, RacerHudRect(fw, fh));
        UploadRacerPanelRect(banner_, cmd, RacerBannerRect(fw, fh));
    }
    const std::string text = FormatRacerHud(state_->race, state_->pod);
    if (text != shown_) {
        UploadRacerPanelText(hud_, cmd, RacerHudLines(text), 0);
        shown_ = text;
    }
    DrawRacerPanel(hud_, cmd, swapchain, res->pipeline, res->sampler);
    const std::string banner = FormatRacerBanner(state_->race);
    if (banner.empty()) return;
    if (banner != bannerShown_) {
        const float x = 0.5f * (kBannerWidth - 8.f * banner.size());
        UploadRacerPanelText(banner_, cmd,
                             {{banner, {255, 225, 90, 255}, x, 1.f}}, 0);
        bannerShown_ = banner;
    }
    DrawRacerPanel(banner_, cmd, swapchain, res->pipeline, res->sampler);
}

}  // namespace sdl3cpp::services::impl

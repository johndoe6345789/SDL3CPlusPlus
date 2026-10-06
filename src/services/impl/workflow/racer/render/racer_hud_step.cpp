#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include "services/interfaces/workflow/rendering/gpu_text_overlay_resources.hpp"

#include <sstream>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

// Five lines of SDL's 8 px font, shown at 3x in the top-right corner.
constexpr int kHudWidth = 320;
constexpr int kHudHeight = 54;
constexpr float kHudScale = 3.f;
constexpr float kMargin = 24.f;

RacerScreenRect TopRight(float frameWidth, float frameHeight) {
    RacerScreenRect r;
    r.right = 1.f - 2.f * kMargin / frameWidth;
    r.left = r.right - 2.f * kHudScale * kHudWidth / frameWidth;
    r.top = 1.f - 2.f * kMargin / frameHeight;
    r.bottom = r.top - 2.f * kHudScale * kHudHeight / frameHeight;
    return r;
}

std::vector<RacerPanelLine> Lines(const std::string& text) {
    std::vector<RacerPanelLine> lines;
    std::istringstream in(text);
    std::string line;
    for (float y = 2.f; std::getline(in, line); y += 10.f) {
        lines.push_back({line, {255, 220, 50, 255}, 4.f, y});
    }
    return lines;
}

}  // namespace

WorkflowRacerHudStep::WorkflowRacerHudStep(
    std::shared_ptr<ILogger> logger, std::shared_ptr<RacerWorldState> state)
    : logger_(std::move(logger)), state_(std::move(state)) {}

WorkflowRacerHudStep::~WorkflowRacerHudStep() { DestroyRacerPanel(hud_); }

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
    if (!hud_.texture) {
        if (!CreateRacerTextPanel(res->device, kHudWidth, kHudHeight, hud_)) {
            return;
        }
        UploadRacerPanelRect(
            hud_, cmd,
            TopRight(static_cast<float>(
                         context.Get<uint32_t>("frame_width", 1920u)),
                     static_cast<float>(
                         context.Get<uint32_t>("frame_height", 1080u))));
    }
    const std::string text = FormatRacerHud(state_->race, state_->pod);
    if (text != shown_) {
        UploadRacerPanelText(hud_, cmd, Lines(text), 0);
        shown_ = text;
    }
    DrawRacerPanel(hud_, cmd, swapchain, res->pipeline, res->sampler);
}

}  // namespace sdl3cpp::services::impl

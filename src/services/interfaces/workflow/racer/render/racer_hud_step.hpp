#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/workflow/racer/render/racer_panel.hpp"
#include "services/interfaces/workflow/racer/world/racer_world_state.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/// HUD: five lines of SDL's 8 px font at 3x, top right. Banner: up to
/// twelve characters at 14x, upper middle.
inline constexpr int kHudWidth = 320;
inline constexpr int kHudHeight = 54;
inline constexpr float kHudScale = 3.f;
inline constexpr float kHudMargin = 24.f;
inline constexpr int kBannerWidth = 96;
inline constexpr int kBannerHeight = 10;
inline constexpr float kBannerScale = 14.f;

/// The HUD lines for a race state: position and lap, times, speed,
/// engine heat, and each engine's health. Pure, so it is testable.
std::string FormatRacerHud(const RacerRaceState& race,
                           const RacerPodState& pod);

/// The big centred banner for a race state, or nothing: the countdown,
/// GO!, FINAL LAP as the last lap starts, and FINISHED.
std::string FormatRacerBanner(const RacerRaceState& race);

/// Where the HUD and the banner go on a frame of the given size.
RacerScreenRect RacerHudRect(float frameWidth, float frameHeight);
RacerScreenRect RacerBannerRect(float frameWidth, float frameHeight);

/// The HUD text as panel lines, one per newline-separated line.
std::vector<RacerPanelLine> RacerHudLines(const std::string& text);

/**
 * Plugin ID: racer.hud.text
 *
 * Draws the HUD in the top-right corner and the race banner in the upper
 * middle, borrowing the engine's text-overlay pipeline (run
 * overlay.fps_init first). Only while racing; after frame.gpu.end_scene.
 */
class WorkflowRacerHudStep final : public IWorkflowStep {
public:
    WorkflowRacerHudStep(std::shared_ptr<ILogger> logger,
                         std::shared_ptr<RacerWorldState> state);
    ~WorkflowRacerHudStep() override;
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<RacerWorldState> state_;
    std::string shown_;
    RacerPanel hud_;
    RacerPanel banner_;
    std::string bannerShown_;
};

}  // namespace sdl3cpp::services::impl

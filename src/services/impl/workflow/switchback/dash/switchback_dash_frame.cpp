#include "services/interfaces/workflow/switchback/dash/switchback_dash_frame.hpp"

#include "services/interfaces/workflow/gta5/hud/gta5_map_frame.hpp"

#include <algorithm>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kKilometresPerMile = 1.609344f;
constexpr float kMphTop = 200.f;
constexpr float kMphStep = 20.f;
constexpr float kRpmTop = 8.f;
constexpr float kRpmStep = 1.f;
constexpr float kGearScale = 10.f;
constexpr float kGearGap = 12.f;
constexpr float kGlyphRows = 7.f;
constexpr float kGlyphColumns = 5.f;
constexpr float kGlyphAdvance = 6.f;
constexpr float kCheckpointScale = 3.f;
constexpr float kFinishScale = 5.f;
constexpr glm::vec2 kCheckpointAt(24.f, 24.f);

void AddGearReadout(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                    const Gta5Hud& hud, glm::vec2 speedo, float radius,
                    int gear) {
    const float text_width = kGlyphColumns * kGearScale;
    const float centre_x = speedo.x - 1.075f * radius;
    const float top = speedo.y - radius - kGlyphRows * kGearScale - kGearGap;
    AddGta5MapText(frame, layout, hud.overlay,
                   glm::vec2(centre_x - text_width * 0.5f, top), kGearScale,
                   std::to_string(gear));
}

void AddCheckpointReadout(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                          const Gta5Hud& hud,
                          const SwitchbackRaceProgress& race) {
    const std::string count = std::to_string(race.passed) + " OF " +
                              std::to_string(race.total);
    AddGta5MapText(frame, layout, hud.overlay, kCheckpointAt,
                   kCheckpointScale, "CHECKPOINT " + count);
    if (!race.finished) return;
    const float below =
        kCheckpointAt.y + kGlyphRows * kCheckpointScale + kGearGap;
    AddGta5MapText(frame, layout, hud.overlay,
                   glm::vec2(kCheckpointAt.x, below), kFinishScale,
                   "FINISHED");
}

}  // namespace

Gta5MapFrame BuildSwitchbackDashFrame(const Gta5Hud& hud, int width,
                                      int height, const Gta5HudState& state,
                                      const SwitchbackRaceProgress& race) {
    Gta5MapFrame frame;
    if (!state.driving) return frame;
    const Gta5MapLayout layout = FitGta5Map(width, height);
    const float w = float(width);
    const float h = float(height);
    const float radius = std::clamp(h * 0.14f, 70.f, 130.f);
    const glm::vec2 speedo(w - 24.f - radius, h - 24.f - radius);
    const float mph = state.kmh / kKilometresPerMile;
    AddGta5HudGauge(frame, layout, hud, hud.speedDial, speedo, radius, mph,
                    kMphTop, kMphStep,
                    std::to_string(int(mph)) + " MPH");
    const float thousands = 0.8f + state.revs * 6.2f;  // idle 800 to 7000
    AddGta5HudGauge(frame, layout, hud, hud.tachDial,
                    speedo - glm::vec2(2.15f * radius, 0.f), radius,
                    thousands, kRpmTop, kRpmStep, "RPM x1000");
    AddGearReadout(frame, layout, hud, speedo, radius, state.gear);
    AddCheckpointReadout(frame, layout, hud, race);
    return frame;
}

}  // namespace sdl3cpp::services::impl

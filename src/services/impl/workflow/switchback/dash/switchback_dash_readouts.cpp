#include "services/interfaces/workflow/switchback/dash/switchback_dash_readouts.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

constexpr float kCheckpointScale = 3.f;
constexpr float kFinishScale = 5.f;
constexpr float kEscScale = 2.f;
constexpr float kEscInset = 24.f;
constexpr glm::vec2 kCheckpointAt(24.f, 24.f);

}  // namespace

void AddCheckpointReadout(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                          const Gta5Hud& hud,
                          const SwitchbackRaceProgress& race) {
    if (!race.onRoute) return;
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
    const float hint = below + kGlyphRows * kFinishScale + kGearGap;
    AddGta5MapText(frame, layout, hud.overlay,
                   glm::vec2(kCheckpointAt.x, hint), kCheckpointScale,
                   "R TO RESTART");
}

void AddEscHint(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                const Gta5Hud& hud, float height) {
    const float top = height - kEscInset - kGlyphRows * kEscScale;
    AddGta5MapText(frame, layout, hud.overlay, glm::vec2(kEscInset, top),
                   kEscScale, "ESC MENU");
}

}  // namespace sdl3cpp::services::impl

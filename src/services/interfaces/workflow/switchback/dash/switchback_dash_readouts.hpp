#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"
#include "services/interfaces/workflow/switchback/dash/switchback_dash_frame.hpp"

namespace sdl3cpp::services::impl {

inline constexpr float kGlyphRows = 7.f;
inline constexpr float kGearGap = 12.f;

/// CHECKPOINT n OF m top left; FINISHED and the restart hint once the route
/// is done. Draws nothing while free roaming.
void AddCheckpointReadout(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                          const Gta5Hud& hud,
                          const SwitchbackRaceProgress& race);

/// ESC MENU bottom left, so the menu can be found during a race.
void AddEscHint(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                const Gta5Hud& hud, float height);

}  // namespace sdl3cpp::services::impl

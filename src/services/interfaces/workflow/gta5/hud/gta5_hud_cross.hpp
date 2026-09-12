#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

namespace sdl3cpp::services::impl {

/// Where the round will go: four bars about a gap in the middle, drawn
/// only with something in hand and nothing else over the screen.
void AddGta5HudCrosshair(Gta5MapFrame& frame, const Gta5MapLayout& layout,
                         const Gta5Hud& hud, const Gta5HudState& state,
                         float w, float h);

}  // namespace sdl3cpp::services::impl

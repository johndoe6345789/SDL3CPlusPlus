#pragma once

#include "services/interfaces/workflow/gta5/hud/gta5_hud.hpp"

namespace sdl3cpp::services::impl {

/// How far the race has got, as the checkpoint step publishes it.
struct SwitchbackRaceProgress {
    int passed{0};
    int total{0};
    bool finished{false};
};

/// Switchback's dashboard: an MPH dial and an RPM dial, bottom right, the
/// gear between them, and the checkpoint count top left. Empty unless the
/// player is seated in the car.
Gta5MapFrame BuildSwitchbackDashFrame(const Gta5Hud& hud, int width,
                                      int height, const Gta5HudState& state,
                                      const SwitchbackRaceProgress& race);

}  // namespace sdl3cpp::services::impl

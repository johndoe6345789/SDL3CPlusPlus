#include "services/interfaces/workflow/racer/player/racer_surface_effects.hpp"

#include "services/interfaces/workflow/racer/data/racer_surface_flags.hpp"

namespace sdl3cpp::services::impl {

RacerSurfaceEffect RacerSurfaceEffectFor(std::uint32_t flags) {
    RacerSurfaceEffect e;
    auto has = [flags](RacerSurfaceFlag f) {
        return HasRacerSurfaceFlag(flags, f);
    };
    if (has(RacerSurfaceFlag::Fast)) e.topSpeed *= 1.25f;
    if (has(RacerSurfaceFlag::Slow)) e.topSpeed *= 0.75f;
    if (has(RacerSurfaceFlag::Rough)) e.topSpeed *= 0.9f;
    if (has(RacerSurfaceFlag::Wet) || has(RacerSurfaceFlag::Swamp)) {
        e.topSpeed *= 0.88f;
    }
    if (has(RacerSurfaceFlag::Slip)) e.grip *= 0.45f;
    if (has(RacerSurfaceFlag::Lava)) e.heatPerSecond = 0.22f;
    e.fatal = has(RacerSurfaceFlag::Fall);
    return e;
}

}  // namespace sdl3cpp::services::impl

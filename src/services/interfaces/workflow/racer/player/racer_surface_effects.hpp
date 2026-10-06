#pragma once

#include <cstdint>

namespace sdl3cpp::services::impl {

/// How a surface's reaction flags change a pod riding it. Speed strips
/// raise the top speed; sand, rough ground, water and swamp drag on it;
/// ice cuts grip; lava heats the engines.
struct RacerSurfaceEffect {
    float topSpeed = 1.f;    ///< factor on the pod's top speed
    float grip = 1.f;        ///< factor on its turn rate
    float heatPerSecond = 0.f;
    bool fatal = false;      ///< a fall the pod cannot come back from
};

RacerSurfaceEffect RacerSurfaceEffectFor(std::uint32_t flags);

}  // namespace sdl3cpp::services::impl

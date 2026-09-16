#include "services/interfaces/workflow/gta5/audio/gta5_sound_step.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5SoundStep::Strokes(float speed, float dt) {
    std::uniform_real_distribution<float> vary(0.9f, 1.1f);
    // A stroke every 1.6 m, or slowly, treading water.
    strokePhase_ += std::max(speed / 1.6f, 0.3f) * dt;
    if (strokePhase_ < 1.f) return;
    strokePhase_ -= 1.f;
    const float gain = speed > 0.5f ? 0.45f : 0.2f;
    PlayGta5Clip(playing_, sounds_.strokes, rng_, device_, spec_,
                 gain * volume_, vary(rng_));
}

}  // namespace sdl3cpp::services::impl

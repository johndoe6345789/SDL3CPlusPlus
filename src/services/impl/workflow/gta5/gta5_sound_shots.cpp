#include "services/interfaces/workflow/gta5/gta5_sound_step.hpp"

#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5SoundStep::Shots(WorkflowContext& context) {
    std::uniform_real_distribution<float> vary(0.95f, 1.05f);
    // Every shot and every blast the weapon counted since last frame,
    // one sound each, up to a handful so a minigun cannot drown the rest.
    const int shots = context.Get<int>("gta5.weapon.shot", 0);
    for (int i = shots_; i < shots && i < shots_ + 4; ++i) {
        PlayGta5Clip(playing_, sounds_.shots, rng_, device_, spec_,
                     0.55f * volume_, vary(rng_));
    }
    shots_ = shots;
    const int blasts = context.Get<int>("gta5.weapon.blast", 0);
    for (int i = blasts_; i < blasts && i < blasts_ + 2; ++i) {
        PlayGta5Clip(playing_, sounds_.blasts, rng_, device_, spec_,
                     0.9f * volume_, vary(rng_));
    }
    blasts_ = blasts;
}

}  // namespace sdl3cpp::services::impl

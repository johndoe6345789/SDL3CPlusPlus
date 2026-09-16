#include "services/interfaces/workflow/gta5/audio/gta5_sound_step.hpp"

#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5SoundStep::Water(WorkflowContext& context) {
    if (sounds_.water.empty()) return;
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    const float under = context.Get<float>("gta5.camera.underwater", -1.f);
    float surface = 0.f, gain = 0.f, ratio = 1.f;
    // Lapping within 8 m above water (GTA's y is engine -z), louder in
    // it; under it, a deep, muffled wash.
    if (under > 0.f) {
        gain  = 0.6f;
        ratio = 0.55f;
    } else if (Gta5WaterHeightAt(water_, ps.origin.x, -ps.origin.z,
                                 surface)) {
        const float above = ps.origin.y + ps.mins.y - surface;
        gain              = 0.35f * std::clamp(1.f - above / 8.f, 0.f, 1.f);
        if (context.GetBool("gta5.swimming", false)) gain = 0.5f;
    }
    FeedGta5Loop(waterLoop_, sounds_.water.front(), device_, spec_,
                 gain * volume_, ratio);
}

}  // namespace sdl3cpp::services::impl

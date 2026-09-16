#include "services/interfaces/workflow/gta5/audio/gta5_sound_step.hpp"

#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace sdl3cpp::services::impl {

void WorkflowGta5SoundStep::Ambience(WorkflowContext& context, float dt) {
    if (ambience_.zones.empty()) return;
    const auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    Gta5Listener listener;
    // GTA's own axes: x east, y north (engine -z), z up (engine y).
    listener.at      = glm::vec3(ps.origin.x, -ps.origin.z, ps.origin.y);
    listener.minutes = context.Get<float>("gta5.time.hours", 12.f) * 60.f;
    listener.dt      = dt;
    listener.volume  = volume_;
    PlayGta5Ambience(ambience_, listener, rng_, device_, spec_, playing_);
}

}  // namespace sdl3cpp::services::impl

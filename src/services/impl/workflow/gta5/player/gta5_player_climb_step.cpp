#include "services/interfaces/workflow/gta5/player/gta5_player_climb_step.hpp"

#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

void WorkflowGta5PlayerClimbStep::Execute(const WorkflowStepDefinition&,
                                          WorkflowContext& context) {
    const bool down    = context.GetBool("input.jump", false);
    const bool pressed = down && !held_;
    held_              = down;
    auto* live         = context.TryGet<Q3PlayerState>("q3.ps");
    if (!state_ || !live || state_->seated >= 0 ||
        context.GetBool("gta5.swimming", false)) {
        climb_.active = false;
        return;
    }
    Q3PlayerState ps = *live;
    const float dt =
        std::clamp(context.Get<float>("physics_dt", 1.f / 60.f), 0.f, 0.1f);
    if (climb_.active) {
        climb_.t      = std::min(climb_.t + dt, climb_.duration);
        ps.origin     = Gta5ClimbAt(climb_);
        ps.velocity   = glm::vec3(0.f);
        ps.onGround   = climb_.t >= climb_.duration;
        climb_.active = !ps.onGround;
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }
    // From standing, or the jump this frame began: its lift is undone
    // below if a climb takes over.
    if (!pressed || ps.velocity.y > 5.5f) return;
    const glm::vec3 feet = ps.origin + glm::vec3(0.f, ps.mins.y, 0.f);
    Begin(context, ps.origin, feet, ps.maxs.y - ps.mins.y);
}

}  // namespace sdl3cpp::services::impl

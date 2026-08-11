#include "services/interfaces/workflow/quake3/workflow_q3_missiles_move_step.hpp"
#include "services/interfaces/workflow/quake3/q3_missile_types.hpp"

#include <string>

namespace sdl3cpp::services::impl {

WorkflowQ3MissilesMoveStep::WorkflowQ3MissilesMoveStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MissilesMoveStep::GetPluginId() const {
    return "q3.missiles.move";
}

void WorkflowQ3MissilesMoveStep::Execute(const WorkflowStepDefinition& /*step*/, WorkflowContext& context) {
    auto missiles = context.Get<sdl3cpp::q3::MissileList>("q3.missiles", nullptr);
    if (!missiles || missiles->empty()) {
        return;
    }

    const float dt = static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    for (auto& m : *missiles) {
        if (m.exploded) continue;

        // Apply gravity to grenades.
        if (m.type == sdl3cpp::q3::MissileType::Grenade) {
            m.velocity.y -= 20.f * dt;
        }

        m.origin      += m.velocity * dt;
        m.lifetimeLeft -= dt;

        if (m.lifetimeLeft <= 0.f) {
            m.exploded = true;
        }
    }

    // No need to re-set — missiles is a shared_ptr and was modified in-place.
    // Set it back so any step that reads the pointer sees the updated list.
    context.Set("q3.missiles", missiles);
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/quake3/workflow_q3_movers_update_step.hpp"
#include "services/interfaces/workflow/quake3/q3_mover_kinematics.hpp"
#include "services/interfaces/workflow/quake3/q3_mover_types.hpp"
#include "services/interfaces/workflow/quake3/q3_player_position.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

WorkflowQ3MoversUpdateStep::WorkflowQ3MoversUpdateStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3MoversUpdateStep::GetPluginId() const {
    return "q3.movers.update";
}

void WorkflowQ3MoversUpdateStep::Execute(const WorkflowStepDefinition&,
                                         WorkflowContext& context) {
    const auto* moversPtr = context.TryGet<sdl3cpp::q3::MoverList>("q3.movers");
    if (!moversPtr || !(*moversPtr)) return;

    auto& movers = **moversPtr;
    if (movers.empty()) return;

    const float dt =
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016));
    if (dt <= 0.f) return;

    const glm::vec3 playerPos = ResolveQ3PlayerPosition(context);

    glm::vec3 velocityPush(0.f);
    for (auto& m : movers) {
        velocityPush += UpdateQ3Mover(m, playerPos, dt);
    }

    // Write push back (additive with any existing push)
    const glm::vec3 existing =
        context.Get<glm::vec3>("q3.player_velocity_push", glm::vec3(0.f));
    context.Set("q3.player_velocity_push", existing + velocityPush);
}

}  // namespace sdl3cpp::services::impl

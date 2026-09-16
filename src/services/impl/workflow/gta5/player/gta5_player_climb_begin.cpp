#include "services/interfaces/workflow/gta5/player/gta5_player_climb_step.hpp"

#include "services/interfaces/workflow/gta5/player/gta5_look.hpp"
#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_input.hpp"
#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <string>

namespace sdl3cpp::services::impl {

void WorkflowGta5PlayerClimbStep::Begin(WorkflowContext& context,
                                        const glm::vec3& origin,
                                        const glm::vec3& feet,
                                        float height) {
    glm::vec3 forward =
        Gta5LookFront(context.Get<float>("camera_yaw", 0.f), 0.f);
    Gta5Ledge ledge;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!FindGta5Ledge(world, Gta5PlayerBody(context), feet, forward,
                       height, ledge)) {
        return;
    }
    climb_.active    = true;
    climb_.t         = 0.f;
    climb_.from      = origin;
    climb_.to        = ledge.top + (origin - feet) + glm::vec3(0, 0.02f, 0);
    climb_.duration  = 0.3f + ledge.rise * 0.15f;
    Q3PlayerState ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    ps.velocity      = glm::vec3(0.f);  // no jump on top of the climb
    context.Set("q3.ps", ps);
    if (logger_) {
        logger_->Info("gta5.player.climb: up " +
                      std::to_string(ledge.rise).substr(0, 4) + " m");
    }
}

}  // namespace sdl3cpp::services::impl

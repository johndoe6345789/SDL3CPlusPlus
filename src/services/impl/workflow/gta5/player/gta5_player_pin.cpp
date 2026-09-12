#include "services/interfaces/workflow/gta5/player/gta5_player_pin.hpp"

#include "services/interfaces/workflow/gta5/vehicle/gta5_vehicle_hold.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"

namespace sdl3cpp::services::impl {

void PinGta5Player(WorkflowContext& context, btRigidBody* body,
                   const glm::vec3& at) {
    btTransform transform = body->getWorldTransform();
    transform.setOrigin(btVector3(at.x, at.y, at.z));
    body->setWorldTransform(transform);
    if (body->getMotionState()) {
        body->getMotionState()->setWorldTransform(transform);
    }
    body->setLinearVelocity(btVector3(0.f, 0.f, 0.f));
    if (!context.Contains("q3.ps")) return;
    auto ps = context.Get<Q3PlayerState>("q3.ps", Q3PlayerState{});
    ps.origin = at;
    ps.velocity = glm::vec3(0.f);
    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", at);
}

bool DropGta5PlayerToGround(WorkflowContext& context, btRigidBody* body,
                            const glm::vec3& at) {
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    float ground = 0.f;
    // The search starts 5 m above its point, so asking from 6 m down
    // starts it a metre under the player -- clear of the capsule, which
    // would otherwise be the first thing the ray meets.
    if (!Gta5GroundBelow(world, at - glm::vec3(0.f, 6.f, 0.f), ground)) {
        return false;
    }
    PinGta5Player(context, body, glm::vec3(at.x, ground + 1.f, at.z));
    return true;
}

}  // namespace sdl3cpp::services::impl

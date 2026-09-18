#include "services/interfaces/workflow/bl4/bl4_player_pin.hpp"

#include "services/interfaces/workflow/quake3/pmove/q3_pm_types.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

void PinBl4Player(WorkflowContext& context, const glm::vec3& origin) {
    const auto name = context.GetString("physics_player_body", "");
    auto* body = name.empty()
                     ? nullptr
                     : context.Get<btRigidBody*>("physics_body_" + name, nullptr);
    if (body) {
        btTransform transform = body->getWorldTransform();
        transform.setOrigin(btVector3(origin.x, origin.y, origin.z));
        body->setWorldTransform(transform);
        if (auto* motion = body->getMotionState()) motion->setWorldTransform(transform);
        body->setLinearVelocity(btVector3(0, 0, 0));
        body->setAngularVelocity(btVector3(0, 0, 0));
    }
    if (auto ps = context.TryGet<Q3PlayerState>("q3.ps")) {
        Q3PlayerState moved = *ps;
        moved.origin = origin;
        moved.velocity = glm::vec3(0.f);
        context.Set("q3.ps", moved);
    }
}

}  // namespace sdl3cpp::services::impl

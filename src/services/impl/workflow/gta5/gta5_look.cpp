#include "services/interfaces/workflow/gta5/gta5_look.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// A ray that passes through the player's own capsule.
struct PastPlayer final : btCollisionWorld::ClosestRayResultCallback {
    PastPlayer(const btVector3& from, const btVector3& to,
               const btCollisionObject* player)
        : ClosestRayResultCallback(from, to), player_(player) {}
    bool needsCollision(btBroadphaseProxy* proxy) const override {
        return proxy->m_clientObject != player_ &&
               ClosestRayResultCallback::needsCollision(proxy);
    }
    const btCollisionObject* player_;
};

}  // namespace

glm::vec3 Gta5ClearEye(btDiscreteDynamicsWorld* world, const glm::vec3& head,
                       const glm::vec3& eye, const btCollisionObject* player) {
    if (!world || glm::length(eye - head) < 1e-3f) return eye;
    PastPlayer hit(btVector3(head.x, head.y, head.z),
                   btVector3(eye.x, eye.y, eye.z), player);
    world->rayTest(hit.m_rayFromWorld, hit.m_rayToWorld, hit);
    if (!hit.hasHit()) return eye;
    const btVector3& at = hit.m_hitPointWorld;
    // 20 cm short, so the near plane does not cut into the wall.
    return glm::vec3(at.x(), at.y(), at.z()) +
           glm::normalize(head - eye) * 0.2f;
}

}  // namespace sdl3cpp::services::impl

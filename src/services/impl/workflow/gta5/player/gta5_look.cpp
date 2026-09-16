#include "services/interfaces/workflow/gta5/player/gta5_look.hpp"

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

/// A swept sphere that passes through the player's own capsule.
struct SweepPastPlayer final
    : btCollisionWorld::ClosestConvexResultCallback {
    SweepPastPlayer(const btVector3& from, const btVector3& to,
                    const btCollisionObject* player)
        : ClosestConvexResultCallback(from, to), player_(player) {}
    bool needsCollision(btBroadphaseProxy* proxy) const override {
        return proxy->m_clientObject != player_ &&
               ClosestConvexResultCallback::needsCollision(proxy);
    }
    const btCollisionObject* player_;
};

}  // namespace

// A sphere, not a ray: the near plane reaches ~0.8 m to its corners,
// and a ray that slipped past a wall's edge left that wall filling a
// third of the screen.
glm::vec3 Gta5ClearEye(btDiscreteDynamicsWorld* world, const glm::vec3& head,
                       const glm::vec3& eye, const btCollisionObject* player) {
    if (!world || glm::length(eye - head) < 1e-3f) return eye;
    const btSphereShape sphere(0.45f);
    btTransform from, to;
    from.setIdentity();
    to.setIdentity();
    from.setOrigin(btVector3(head.x, head.y, head.z));
    to.setOrigin(btVector3(eye.x, eye.y, eye.z));
    SweepPastPlayer hit(from.getOrigin(), to.getOrigin(), player);
    world->convexSweepTest(&sphere, from, to, hit);
    if (!hit.hasHit()) return eye;
    return head + (eye - head) * hit.m_closestHitFraction;
}

bool Gta5RayHit(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                const glm::vec3& to, const btCollisionObject* skip,
                glm::vec3* at) {
    if (!world) return false;
    PastPlayer hit(btVector3(from.x, from.y, from.z),
                   btVector3(to.x, to.y, to.z), skip);
    world->rayTest(hit.m_rayFromWorld, hit.m_rayToWorld, hit);
    if (hit.hasHit() && at) {
        const btVector3& p = hit.m_hitPointWorld;
        *at = glm::vec3(p.x(), p.y(), p.z());
    }
    return hit.hasHit();
}

}  // namespace sdl3cpp::services::impl

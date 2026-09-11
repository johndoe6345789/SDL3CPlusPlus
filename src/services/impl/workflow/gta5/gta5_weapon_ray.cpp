#include "services/interfaces/workflow/gta5/gta5_weapon_ray.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {

namespace {

/// The player stands between the camera and whatever is being shot at,
/// so their own capsule is passed through rather than counted as cover.
struct PastOne : public btCollisionWorld::ClosestRayResultCallback {
    const btCollisionObject* skip{nullptr};
    PastOne(const btVector3& from, const btVector3& to)
        : ClosestRayResultCallback(from, to) {}
    bool needsCollision(btBroadphaseProxy* proxy) const override {
        return proxy->m_clientObject != skip &&
               ClosestRayResultCallback::needsCollision(proxy);
    }
};

}  // namespace

bool Gta5ShootRay(btDiscreteDynamicsWorld* world, const glm::vec3& from,
                  const glm::vec3& to, const btCollisionObject* skip,
                  Gta5Shot& shot) {
    if (!world) return false;
    const btVector3 start(from.x, from.y, from.z);
    const btVector3 end(to.x, to.y, to.z);
    PastOne hit(start, end);
    hit.skip = skip;
    world->rayTest(start, end, hit);
    if (!hit.hasHit()) return false;
    shot.at = glm::vec3(hit.m_hitPointWorld.x(), hit.m_hitPointWorld.y(),
                        hit.m_hitPointWorld.z());
    shot.normal = glm::vec3(hit.m_hitNormalWorld.x(), hit.m_hitNormalWorld.y(),
                            hit.m_hitNormalWorld.z());
    shot.object = hit.m_collisionObject;
    return true;
}

}  // namespace sdl3cpp::services::impl

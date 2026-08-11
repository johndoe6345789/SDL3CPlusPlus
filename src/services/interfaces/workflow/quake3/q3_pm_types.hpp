#pragma once

#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>

namespace sdl3cpp::services::impl {

// ─────────────────────────────────────────────────────────────────────────────
// Q3PlayerState
//   Authoritative per-frame player movement state.
//   Context key: "q3.ps"  (stored by value as Q3PlayerState)
// ─────────────────────────────────────────────────────────────────────────────
struct Q3PlayerState {
    glm::vec3 origin{0.f, 1.f, 0.f};
    glm::vec3 velocity{0.f, 0.f, 0.f};
    glm::vec3 mins{-0.28f,  0.f,  -0.28f};
    glm::vec3 maxs{ 0.28f,  1.4f,  0.28f};
    bool  onGround{false};
    bool  crouching{false};
    float groundFraction{0.f};   // 0 = air, 1 = fully grounded
};

// ─────────────────────────────────────────────────────────────────────────────
// Q3Trace
//   Result from a swept-box trace against the Bullet world.
// ─────────────────────────────────────────────────────────────────────────────
struct Q3Trace {
    bool      hit{false};
    float     fraction{1.f};   // 0 = started solid, 1 = no hit
    glm::vec3 endPos{0.f};
    glm::vec3 normal{0.f, 1.f, 0.f};
    bool      startSolid{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// Callback that ignores the kinematic player ghost itself (if any).
// We use the simplest form: closest-hit convex result.
// ─────────────────────────────────────────────────────────────────────────────
struct Q3NotMeCallback final : public btCollisionWorld::ClosestConvexResultCallback {
    const btCollisionObject* me{nullptr};

    Q3NotMeCallback()
        : btCollisionWorld::ClosestConvexResultCallback(
              btVector3(0, 0, 0), btVector3(0, 0, 0)) {}

    btScalar addSingleResult(
        btCollisionWorld::LocalConvexResult& result,
        bool normalInWorldSpace) override
    {
        if (result.m_hitCollisionObject == me) return 1.f;
        return ClosestConvexResultCallback::addSingleResult(result, normalInWorldSpace);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// TraceBox
//   Sweeps an AABB (defined by mins/maxs) from `from` to `to` in the given
//   Bullet world and returns collision info.
// ─────────────────────────────────────────────────────────────────────────────
inline Q3Trace TraceBox(
    btDiscreteDynamicsWorld* world,
    glm::vec3 from,
    glm::vec3 to,
    glm::vec3 mins,
    glm::vec3 maxs)
{
    Q3Trace result;
    result.endPos = to;

    if (!world) return result;

    // Half-extents of the AABB
    const glm::vec3 half = (maxs - mins) * 0.5f;
    btBoxShape boxShape(btVector3(half.x, half.y, half.z));
    boxShape.setMargin(0.001f);

    // Centre offset of the AABB (mins are not necessarily symmetric)
    const glm::vec3 centre = (mins + maxs) * 0.5f;

    btTransform fromTr, toTr;
    fromTr.setIdentity();
    toTr.setIdentity();
    fromTr.setOrigin(btVector3(from.x + centre.x, from.y + centre.y, from.z + centre.z));
    toTr.setOrigin(btVector3(to.x + centre.x, to.y + centre.y, to.z + centre.z));

    Q3NotMeCallback cb;
    cb.m_collisionFilterGroup = btBroadphaseProxy::DefaultFilter;
    cb.m_collisionFilterMask  = btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter;

    world->convexSweepTest(&boxShape, fromTr, toTr, cb, 0.001f);

    if (cb.hasHit()) {
        result.hit      = true;
        result.fraction = cb.m_closestHitFraction;
        const btVector3& n = cb.m_hitNormalWorld;
        result.normal   = glm::vec3(n.x(), n.y(), n.z());

        // Interpolate end position along the sweep
        glm::vec3 delta = to - from;
        result.endPos   = from + delta * result.fraction;
    }

    return result;
}

}  // namespace sdl3cpp::services::impl

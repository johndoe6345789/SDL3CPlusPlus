#pragma once

#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow_context.hpp"

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
    glm::vec3 mins{-q3::kPlayerHalfWidth, q3::kPlayerFeet,
                   -q3::kPlayerHalfWidth};
    glm::vec3 maxs{ q3::kPlayerHalfWidth, q3::kPlayerHead,
                    q3::kPlayerHalfWidth};
    bool  onGround{false};
    bool  crouching{false};
    float groundFraction{0.f};   // 0 = air, 1 = fully grounded
    /// Plane of the surface being stood on, as ioq3 keeps in
    /// pml.groundTrace. Movement is projected onto it so that walking
    /// on a slope pushes along the slope instead of into it. Straight
    /// up whenever not grounded.
    glm::vec3 groundNormal{0.f, 1.f, 0.f};
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
// PlayerBody
//   The player's own collision object, which pmove traces must ignore.
//   Null when the package has no player body, as the tests do.
// ─────────────────────────────────────────────────────────────────────────────
inline const btCollisionObject* PlayerBody(const WorkflowContext& context) {
    const auto name = context.GetString("physics_player_body", "");
    if (name.empty()) return nullptr;
    return context.Get<btRigidBody*>("physics_body_" + name, nullptr);
}

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
    glm::vec3 maxs,
    const btCollisionObject* ignore = nullptr)
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
    // Without this the sweep hits the player's own capsule, which
    // q3.player.commit has just teleported onto the trace's start
    // point, so every move is blocked at fraction 0.
    cb.me = ignore;
    cb.m_collisionFilterGroup = btBroadphaseProxy::DefaultFilter;
    cb.m_collisionFilterMask  = btBroadphaseProxy::StaticFilter | btBroadphaseProxy::DefaultFilter;

    world->convexSweepTest(&boxShape, fromTr, toTr, cb, 0.001f);

    if (cb.hasHit()) {
        result.hit      = true;
        result.fraction = cb.m_closestHitFraction;
        const btVector3& n = cb.m_hitNormalWorld;
        result.normal   = glm::vec3(n.x(), n.y(), n.z());

        // Interpolate end position along the sweep, then back off along
        // the normal by Quake's SURFACE_CLIP_EPSILON (cm_local.h, 0.125
        // units). A Quake trace never ends in contact, so the next sweep
        // never starts in contact; Bullet's does, which stalls the player
        // against every surface they touch. When the sweep began in
        // contact (fraction 0) this also eases them back out.
        constexpr float kSurfaceClipEpsilon = 0.125f / 32.0f;
        glm::vec3 delta = to - from;
        result.endPos   = from + delta * result.fraction
                        + result.normal * kSurfaceClipEpsilon;
    }

    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// GroundProbe
//   "What am I standing on?" answered with a box inset from the player's
//   own width.
//
//   Quake traces brush planes, so a trace always reports the plane of a
//   face. Bullet's sweep reports the minimum separation direction, which
//   where a floor meets a ramp is a diagonal edge normal shallower than
//   either face — read as ground it says "too steep to stand on" at the
//   foot of every slope. Insetting the box clears that seam so the sweep
//   lands on the face actually beneath the player.
//
//   Positioning still uses the player's real box; only the walkable/not
//   decision uses this.
// ─────────────────────────────────────────────────────────────────────────────
inline constexpr float kGroundProbeInset = 0.8f;

inline Q3Trace GroundProbe(
    btDiscreteDynamicsWorld* world,
    glm::vec3 origin,
    float distance,
    glm::vec3 mins,
    glm::vec3 maxs,
    const btCollisionObject* ignore = nullptr)
{
    mins.x *= kGroundProbeInset;
    maxs.x *= kGroundProbeInset;
    mins.z *= kGroundProbeInset;
    maxs.z *= kGroundProbeInset;
    return TraceBox(world, origin, origin - glm::vec3(0.f, distance, 0.f),
                    mins, maxs, ignore);
}

}  // namespace sdl3cpp::services::impl

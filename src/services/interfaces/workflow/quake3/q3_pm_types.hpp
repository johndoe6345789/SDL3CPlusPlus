#pragma once

#include "services/interfaces/workflow/quake3/q3_brush_trace.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>
#include <BulletCollision/CollisionDispatch/btCollisionWorld.h>
#include <BulletCollision/CollisionShapes/btConvexPolyhedron.h>

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
// Callback that ignores the kinematic player ghost itself (if any).
// We use the simplest form: closest-hit convex result.
// ─────────────────────────────────────────────────────────────────────────────
struct Q3NotMeCallback final : public btCollisionWorld::ClosestConvexResultCallback {
    const btCollisionObject* me{nullptr};
    int hitChild{-1};  // compound child index, -1 if none

    Q3NotMeCallback()
        : btCollisionWorld::ClosestConvexResultCallback(
              btVector3(0, 0, 0), btVector3(0, 0, 0)) {}

    btScalar addSingleResult(
        btCollisionWorld::LocalConvexResult& result,
        bool normalInWorldSpace) override
    {
        if (result.m_hitCollisionObject == me) return 1.f;
        const btScalar fraction =
            ClosestConvexResultCallback::addSingleResult(result,
                                                         normalInWorldSpace);
        if (result.m_localShapeInfo) {
            hitChild = result.m_localShapeInfo->m_triangleIndex;
        }
        return fraction;
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

// ─────────────────────────────────────────────────────────────────────
// FaceNormalAt
//   The plane of the face a sweep actually touched.
//
//   Quake traces brush planes, so a trace always reports a face. Bullet
//   reports the minimum separation direction, which at the seam between
//   two faces is a diagonal belonging to neither: at the foot of a ramp
//   it is shallower than the slope and steeper than the floor, so every
//   walkable-or-not decision taken from it is wrong. Everything we
//   collide with is a polyhedron built from planes, so recover the face
//   whose plane the contact point lies on and report that instead.
// ─────────────────────────────────────────────────────────────────────
inline glm::vec3 FaceNormalAt(const btCollisionObject* object, int child,
                              const btVector3& contact,
                              const glm::vec3& swept,
                              const glm::vec3& sweepDir = glm::vec3(0.f)) {
    if (!object) return swept;
    const btCollisionShape* shape = object->getCollisionShape();
    btTransform to = object->getWorldTransform();
    if (shape && shape->isCompound() && child >= 0) {
        const auto* compound =
            static_cast<const btCompoundShape*>(shape);
        if (child >= compound->getNumChildShapes()) return swept;
        to = to * compound->getChildTransform(child);
        shape = compound->getChildShape(child);
    }
    if (!shape || !shape->isPolyhedral()) return swept;
    const auto* poly =
        static_cast<const btPolyhedralConvexShape*>(shape);

    const btVector3 local = to.inverse() * contact;
    // Prefer the sweep's own direction to decide which faces could have
    // stopped it: a face that blocks a move has to point back against
    // it. Bullet's reported hit normal is arbitrary when the sweep
    // starts already in contact, and trusting it there picked a face at
    // right angles to the motion -- a downward probe onto a step came
    // back with the step's side normal, so the player was neither
    // standing on it nor able to fall off it.
    const bool haveDir = glm::dot(sweepDir, sweepDir) > 0.f;
    const btVector3 towards =
        haveDir ? btVector3(-sweepDir.x, -sweepDir.y, -sweepDir.z)
                : btVector3(swept.x, swept.y, swept.z);
    btVector3 best(0.f, 0.f, 0.f);
    btScalar nearest = SIMD_INFINITY;

    // Of the faces pointing back along the sweep, the one whose plane
    // the contact sits on is the one that stopped it.
    const auto consider = [&](const btVector3& n, btScalar d) {
        if (n.dot(towards) <= 0.f) return;
        const btScalar offset = btFabs(local.dot(n) + d);
        if (offset < nearest) {
            nearest = offset;
            best = n;
        }
    };

    if (const btConvexPolyhedron* hull = poly->getConvexPolyhedron()) {
        for (int i = 0; i < hull->m_faces.size(); ++i) {
            const btScalar* plane = hull->m_faces[i].m_plane;
            consider(btVector3(plane[0], plane[1], plane[2]), plane[3]);
        }
    } else {
        for (int i = 0; i < poly->getNumPlanes(); ++i) {
            btVector3 n, support;
            poly->getPlane(n, support, i);
            consider(n, -support.dot(n));
        }
    }
    if (nearest == SIMD_INFINITY) return swept;
    const btVector3 world = to.getBasis() * best;
    return glm::vec3(world.x(), world.y(), world.z());
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

    // Prefer the map's brushes when bsp.build_collision has attached
    // them. Tracing planes is what Quake does, and it answers the two
    // questions a convex sweep cannot: which surface stopped the move,
    // and whether the sweep began inside something. The model hangs off
    // the world because it describes that world, and because every
    // trace already has the world to hand.
    if (const auto* brushes = static_cast<const BrushCollisionModel*>(
            world->getWorldUserInfo())) {
        return TraceBoxThroughBrushes(*brushes, from, to, mins, maxs);
    }

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
    // CharacterFilter carries the map's player-clip brushes. Only pmove
    // traces with this helper, so including it here is Quake's
    // MASK_PLAYERSOLID: players are stopped by clip, shots are not.
    cb.m_collisionFilterMask  = btBroadphaseProxy::StaticFilter |
                                btBroadphaseProxy::DefaultFilter |
                                btBroadphaseProxy::CharacterFilter;

    world->convexSweepTest(&boxShape, fromTr, toTr, cb, 0.001f);

    if (cb.hasHit()) {
        result.hit      = true;
        result.fraction = cb.m_closestHitFraction;
        const btVector3& n = cb.m_hitNormalWorld;
        const glm::vec3 sweep = to - from;
        const float sweepLength = glm::length(sweep);
        result.normal   = FaceNormalAt(cb.m_hitCollisionObject, cb.hitChild,
                                       cb.m_hitPointWorld,
                                       glm::vec3(n.x(), n.y(), n.z()),
                                       sweepLength > 0.f ? sweep / sweepLength
                                                         : glm::vec3(0.f));

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

// ─────────────────────────────────────────────────────────────────────
// GroundProbe
//   "What am I standing on?" — a short downward sweep of the player's
//   box. Named because three steps ask exactly this question and must
//   agree: the ground trace, the never-step-while-rising guard, and the
//   check that a step settled onto something walkable.
// ─────────────────────────────────────────────────────────────────────
inline Q3Trace GroundProbe(btDiscreteDynamicsWorld* world, glm::vec3 origin,
                           float distance, glm::vec3 mins, glm::vec3 maxs,
                           const btCollisionObject* ignore = nullptr) {
    return TraceBox(world, origin, origin - glm::vec3(0.f, distance, 0.f),
                    mins, maxs, ignore);
}

}  // namespace sdl3cpp::services::impl

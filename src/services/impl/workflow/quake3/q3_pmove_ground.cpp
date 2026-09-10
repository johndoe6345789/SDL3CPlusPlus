#include "services/interfaces/workflow/quake3/q3_pmove.hpp"

namespace sdl3cpp::q3 {
namespace {

/// Walkable slope: ioq3 MIN_WALK_NORMAL, cos 45 degrees.
constexpr float kMinGroundNormalY = 0.7f;

/// How far below the origin to look for a floor.
constexpr float kGroundProbe = 0.05f;

}  // namespace

void PmGroundTrace(services::impl::Q3PlayerState& ps,
                   btDiscreteDynamicsWorld* world, float dt,
                   const btCollisionObject* self) {
    bool grounded = false;
    if (world) {
        const services::impl::Q3Trace tr = services::impl::GroundProbe(
            world, ps.origin, kGroundProbe, ps.mins, ps.maxs, self);
        if (tr.hit && tr.normal.y >= kMinGroundNormalY) {
            grounded        = true;
            ps.onGround     = true;
            ps.groundNormal = tr.normal;
            // Deliberately does not move the origin. ioq3's
            // PM_GroundTrace only records the ground plane; snapping the
            // box flush onto the surface makes every horizontal sweep
            // afterwards start in contact, so the trace returns
            // fraction 0, a bump is spent going nowhere, and movement
            // is quietly degraded.
            if (ps.velocity.y < 0.0f) ps.velocity.y = 0.0f;
        }
    }

    if (!grounded) {
        ps.onGround     = false;
        ps.groundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
        ps.velocity.y -= kGravity * dt;
    }

    ps.groundFraction = grounded ? 1.0f : 0.0f;
}

}  // namespace sdl3cpp::q3

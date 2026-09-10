#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"

#include <array>
#include <glm/glm.hpp>

namespace sdl3cpp::q3 {

bool SlideMove(services::impl::Q3PlayerState& ps,
               btDiscreteDynamicsWorld* world, float dt,
               const btCollisionObject* ignore) {
    constexpr int kMaxBumps = 4;
    constexpr float kMinFraction = 0.001f;

    float timeLeft = dt;
    std::array<glm::vec3, kMaxBumps> planes{};
    int numPlanes = 0;
    bool blocked = false;

    for (int bump = 0; bump < kMaxBumps && timeLeft > 0.f; ++bump) {
        const glm::vec3 target = ps.origin + ps.velocity * timeLeft;
        auto tr = services::impl::TraceBox(world, ps.origin, target,
                                           ps.mins, ps.maxs, ignore);
        tr.normal = FaceNormal(tr.normal);
        // Always take the trace's end position. Bullet reports a sweep
        // that starts in contact as a hit at fraction 0, and TraceBox
        // turns that into an end position eased back out along the
        // normal (Quake's SURFACE_CLIP_EPSILON). Guarding the
        // assignment on a minimum fraction threw that escape away in
        // exactly the case it exists for, so a player who ended a move
        // touching a surface could never leave it: every later sweep
        // started in the same contact, returned fraction 0, and moved
        // them nowhere. That is what wedged them against a stair.
        ps.origin = tr.endPos;
        if (tr.fraction > kMinFraction) {
            timeLeft *= (1.f - tr.fraction);
        }
        if (tr.fraction >= 1.f || !tr.hit) {
            break;
        }
        blocked = true;

        // A plane we already hold means the last slide left us a hair
        // inside it; ioq3 nudges out and retries rather than storing a
        // duplicate, which would otherwise cancel the move.
        bool samePlane = false;
        for (int p = 0; p < numPlanes; ++p) {
            if (glm::dot(tr.normal, planes[p]) > kSamePlaneDot) {
                // ioq3 adds the bare unit normal, but its velocities are
                // in Quake units where that is 1/320th of walk speed.
                // In metres it would be 1 m/s, a tenth of walk speed and
                // straight up off a floor, which launched the player.
                ps.velocity += tr.normal * FromQuakeUnits(1.0f);
                samePlane = true;
                break;
            }
        }
        if (samePlane) continue;

        if (numPlanes < kMaxBumps) planes[numPlanes++] = tr.normal;

        ps.velocity = ResolveAgainstPlanes(ps.velocity, planes.data(),
                                           numPlanes);
        if (glm::dot(ps.velocity, ps.velocity) <= 0.f) break;
    }
    return blocked;
}

}  // namespace sdl3cpp::q3

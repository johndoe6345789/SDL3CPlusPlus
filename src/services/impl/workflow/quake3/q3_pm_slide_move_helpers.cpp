#include "services/interfaces/workflow/quake3/q3_pm_slide_move_helpers.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"

#include <array>

namespace sdl3cpp::services::impl {

glm::vec3 ClipVelocity(const glm::vec3& v, const glm::vec3& normal,
                       float overbounce) {
    const float backoff = glm::dot(v, normal) * overbounce;
    return v - normal * backoff;
}

void RunQ3SlideMove(Q3PlayerState& ps, btDiscreteDynamicsWorld* world,
                    const WorkflowContext& context, float dt) {
    static constexpr int kMaxBumps = 4;
    static constexpr float kMinFraction = 0.001f;

    float timeLeft = dt;
    int bumpCount = 0;

    // Accumulate hit normals to handle crease collisions.
    std::array<glm::vec3, kMaxBumps> planes{};
    int numPlanes = 0;

    for (bumpCount = 0; bumpCount < kMaxBumps; ++bumpCount) {
        if (timeLeft <= 0.f) break;

        const glm::vec3 target = ps.origin + ps.velocity * timeLeft;
        Q3Trace tr = TraceBox(world, ps.origin, target, ps.mins, ps.maxs,
                              PlayerBody(context));

        // Advance as far as the trace allows.
        if (tr.fraction > kMinFraction) {
            ps.origin = tr.endPos;
            timeLeft *= (1.f - tr.fraction);
        }

        // No collision this bump: we moved the full remaining distance.
        if (tr.fraction >= 1.f || !tr.hit) break;

        // Hitting a plane we already have means the previous slide left
        // us a hair inside it. ioq3 nudges out along the normal and
        // retries rather than recording a duplicate; without this the
        // plane list fills with copies of the same wall and the move is
        // cancelled, which is what left the player stuck against walls.
        bool samePlane = false;
        for (int p = 0; p < numPlanes; ++p) {
            if (glm::dot(tr.normal, planes[p]) > q3::kSamePlaneDot) {
                ps.velocity += tr.normal;
                samePlane = true;
                break;
            }
        }
        if (samePlane) continue;

        if (numPlanes < kMaxBumps) {
            planes[numPlanes++] = tr.normal;
        }

        // Make the velocity parallel to every plane it is entering.
        // Only a genuine three-plane corner stops the player dead.
        ps.velocity = q3::ResolveAgainstPlanes(ps.velocity, planes.data(),
                                               numPlanes);
        if (glm::dot(ps.velocity, ps.velocity) <= 0.f) break;
    }
}

}  // namespace sdl3cpp::services::impl

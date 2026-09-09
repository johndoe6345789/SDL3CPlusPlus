#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"

#include <array>
#include <glm/glm.hpp>

namespace sdl3cpp::q3 {

bool SlideMove(services::impl::Q3PlayerState& ps,
               btDiscreteDynamicsWorld* world, float dt) {
    constexpr int kMaxBumps = 4;
    constexpr float kMinFraction = 0.001f;

    float timeLeft = dt;
    std::array<glm::vec3, kMaxBumps> planes{};
    int numPlanes = 0;
    bool blocked = false;

    for (int bump = 0; bump < kMaxBumps && timeLeft > 0.f; ++bump) {
        const glm::vec3 target = ps.origin + ps.velocity * timeLeft;
        const auto tr = services::impl::TraceBox(world, ps.origin, target,
                                                 ps.mins, ps.maxs);
        if (tr.fraction > kMinFraction) {
            ps.origin = tr.endPos;
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
                ps.velocity += tr.normal;
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

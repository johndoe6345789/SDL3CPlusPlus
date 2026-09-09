#include "services/interfaces/workflow/quake3/workflow_q3_pm_slide_move_step.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

namespace sdl3cpp::services::impl {

// ─────────────────────────────────────────────────────────────────────────────
// ClipVelocity
//   v = v - dot(v, n) * overbounce * n
//   overbounce > 1 prevents the player from sticking to surfaces.
// ─────────────────────────────────────────────────────────────────────────────
static glm::vec3 ClipVelocity(const glm::vec3& v, const glm::vec3& normal, float overbounce)
{
    const float backoff = glm::dot(v, normal) * overbounce;
    return v - normal * backoff;
}

WorkflowQ3PmSlideMoveStep::WorkflowQ3PmSlideMoveStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmSlideMoveStep::GetPluginId() const { return "q3.pm.slide_move"; }

void WorkflowQ3PmSlideMoveStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context)
{
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const float dt = static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    if (!world) {
        // No physics world: integrate velocity directly
        ps.origin += ps.velocity * dt;
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }

    // ── Slide-move loop ───────────────────────────────────────────────────────
    static constexpr int   kMaxBumps    = 4;
    static constexpr float kOverbounce  = 1.001f;
    static constexpr float kMinFraction = 0.001f;

    float timeLeft = dt;
    int   bumpCount = 0;

    // Accumulate hit normals to handle crease collisions
    std::array<glm::vec3, kMaxBumps> planes{};
    int numPlanes = 0;

    for (bumpCount = 0; bumpCount < kMaxBumps; ++bumpCount) {
        if (timeLeft <= 0.f) break;

        const glm::vec3 target = ps.origin + ps.velocity * timeLeft;
        Q3Trace tr = TraceBox(world, ps.origin, target, ps.mins, ps.maxs);

        // Advance as far as the trace allows
        if (tr.fraction > kMinFraction) {
            ps.origin = tr.endPos;
            timeLeft *= (1.f - tr.fraction);
        }

        // No collision this bump: we moved the full remaining distance
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

    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
    // Yaw is managed by the camera step; just re-publish what is already set.
    // (No-op write to keep the key alive for any downstream reader.)
    context.Set("q3.player_yaw", context.Get<float>("q3.player_yaw", 0.f));
}

}  // namespace sdl3cpp::services::impl

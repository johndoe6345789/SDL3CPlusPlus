#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <glm/glm.hpp>
#include <utility>

namespace sdl3cpp::services::impl {

// Gravity in world units/s² at our Q3 scale (800 q3units * 0.03125 ≈ 25, using 20 as spec'd)
static constexpr float kGravity = 20.f;
// Walkable slope: normal.y must exceed this (cos 45°≈0.7)
static constexpr float kMinGroundNormalY = 0.7f;
// Distance to probe below origin for ground detection
static constexpr float kGroundProbe = 0.05f;

WorkflowQ3PmGroundStep::WorkflowQ3PmGroundStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmGroundStep::GetPluginId() const { return "q3.pm.ground"; }

void WorkflowQ3PmGroundStep::Execute(
    const WorkflowStepDefinition& /*step*/, WorkflowContext& context)
{
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    const float dt = static_cast<float>(context.GetDouble("frame.delta_time", 0.016));
    auto* world    = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);

    // ── Ground detection: trace down kGroundProbe units ──────────────────────
    bool grounded = false;
    if (world) {
        const glm::vec3 traceEnd = ps.origin - glm::vec3(0.f, kGroundProbe, 0.f);
        Q3Trace tr = TraceBox(world, ps.origin, traceEnd, ps.mins, ps.maxs);
        if (tr.hit && tr.normal.y >= kMinGroundNormalY) {
            grounded     = true;
            ps.onGround  = true;
            // Snap origin to ground surface
            ps.origin    = tr.endPos;
            // Kill any downward velocity when landing
            if (ps.velocity.y < 0.f) ps.velocity.y = 0.f;
        }
    }

    if (!grounded) {
        ps.onGround    = false;
        // Apply gravity
        ps.velocity.y -= kGravity * dt;
    }

    ps.groundFraction = grounded ? 1.f : 0.f;

    context.Set("q3.ps", ps);
}

}  // namespace sdl3cpp::services::impl

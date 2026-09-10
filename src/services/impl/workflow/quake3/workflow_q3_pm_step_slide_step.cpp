#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/q3_step_slide.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>

namespace sdl3cpp::services::impl {
namespace {

// A convex-hull-per-brush collision world can leave a hairline gap or
// overlap where two adjacent floor brushes meet, even though the map
// looks perfectly flat there. Bullet's narrowphase then reports that
// seam as a tiny wall, so SlideMove blocks and the seam is too short
// for ComputeStepUpTrace's headroom check to treat as a step: the
// player is walking, on the ground, wants to move, and does not.
// Detect exactly that (grounded, meaningful wish speed, negligible
// horizontal progress) for a few consecutive frames and lift the
// player a hair -- far less than a real step -- so the next frame's
// trace starts clear of the seam instead of re-finding the same one.
constexpr float kStuckHorizontalEpsilon = q3::FromQuakeUnits(0.5f);
constexpr float kStuckWishSpeedMin      = q3::FromQuakeUnits(8.0f);
constexpr int kStuckFrameThreshold      = 6;
constexpr float kUnstickLift            = q3::FromQuakeUnits(2.0f);

void RecoverFromStuckSeam(Q3PlayerState& ps, const glm::vec3& preOrigin,
                          WorkflowContext& context,
                          const std::shared_ptr<ILogger>& logger) {
    const glm::vec3 horizontalDelta(ps.origin.x - preOrigin.x, 0.f,
                                    ps.origin.z - preOrigin.z);
    const glm::vec3 horizontalWish(ps.velocity.x, 0.f, ps.velocity.z);
    const bool triedToMove =
        ps.onGround && glm::length(horizontalWish) > kStuckWishSpeedMin;
    const bool madeNoProgress =
        glm::length(horizontalDelta) < kStuckHorizontalEpsilon;

    const int stuckFrames =
        static_cast<int>(context.GetDouble("q3.stuck_frames", 0.0));
    if (!triedToMove || !madeNoProgress) {
        if (stuckFrames != 0) context.Set<double>("q3.stuck_frames", 0.0);
        return;
    }

    if (stuckFrames + 1 < kStuckFrameThreshold) {
        context.Set<double>("q3.stuck_frames",
                            static_cast<double>(stuckFrames + 1));
        return;
    }

    ps.origin.y += kUnstickLift;
    context.Set<double>("q3.stuck_frames", 0.0);
    if (logger) {
        logger->Warn("q3.pm.step_slide: unstuck nudge applied (grounded, "
                     "moving, but blocked for " +
                     std::to_string(kStuckFrameThreshold) + " frames)");
    }
}

}  // namespace

WorkflowQ3PmStepSlideStep::WorkflowQ3PmStepSlideStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowQ3PmStepSlideStep::GetPluginId() const {
    return "q3.pm.step_slide";
}

void WorkflowQ3PmStepSlideStep::Execute(const WorkflowStepDefinition&,
                                        WorkflowContext& context) {
    auto* psPtr = context.TryGet<Q3PlayerState>("q3.ps");
    if (!psPtr) return;

    Q3PlayerState ps = *psPtr;
    auto* world =
        context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    const float dt =
        static_cast<float>(context.GetDouble("frame.delta_time", 0.016));

    if (!world) {
        ps.origin += ps.velocity * dt;
        context.Set("q3.ps", ps);
        context.Set("q3.player_pos", ps.origin);
        return;
    }

    const glm::vec3 preOrigin     = ps.origin;
    const btCollisionObject* self = PlayerBody(context);
    const auto stepDelta = q3::ApplyQ3StepSlideMove(ps, world, dt, self);

    // A rejected step attempt (nullopt) leaves q3.step_delta untouched,
    // matching the original monolithic step's early returns.
    if (stepDelta) {
        context.Set<float>("q3.step_delta", *stepDelta);
    }

    RecoverFromStuckSeam(ps, preOrigin, context, logger_);

    context.Set("q3.ps", ps);
    context.Set("q3.player_pos", ps.origin);
}

}  // namespace sdl3cpp::services::impl

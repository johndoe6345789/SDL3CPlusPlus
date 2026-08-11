#include "services/interfaces/workflow/workflow_generic_steps/workflow_physics_step_step.hpp"
#include "services/interfaces/workflow/workflow_step_parameter_resolver.hpp"
#include "services/interfaces/workflow_step_definition.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <SDL3/SDL_timer.h>
#include <algorithm>
#include <btBulletDynamicsCommon.h>
#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

WorkflowPhysicsStepStep::WorkflowPhysicsStepStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowPhysicsStepStep::GetPluginId() const {
    return "physics.step";
}

void WorkflowPhysicsStepStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {

    auto* world = context.Get<btDiscreteDynamicsWorld*>("physics_world", nullptr);
    if (!world) {
        throw std::runtime_error("physics.step: No physics world (run physics.world.create first)");
    }

    WorkflowStepParameterResolver paramResolver;
    float dt = 0.0f;  // sentinel: 0 means "compute real dt"
    int maxSubSteps = 10;

    if (const auto* p = paramResolver.FindParameter(step, "delta_time")) {
        if (p->type == WorkflowParameterValue::Type::Number) dt = static_cast<float>(p->numberValue);
    }
    if (const auto* p = paramResolver.FindParameter(step, "max_sub_steps")) {
        if (p->type == WorkflowParameterValue::Type::Number) maxSubSteps = static_cast<int>(p->numberValue);
    }

    // When delta_time isn't pinned in the workflow, derive it from the real
    // wall clock so high-refresh displays don't run physics 4× faster than
    // intended. First call falls back to 1/60. Clamp to [1/600, 1/30] so a
    // hitch (debugger break, alt-tab) can't blow up Bullet's solver.
    if (dt <= 0.0f) {
        const Uint64 now = SDL_GetTicksNS();
        if (last_tick_ns_ != 0) {
            dt = static_cast<float>(static_cast<double>(now - last_tick_ns_) / 1e9);
            dt = std::clamp(dt, 1.0f / 600.0f, 1.0f / 30.0f);
        } else {
            dt = 1.0f / 60.0f;
        }
        last_tick_ns_ = now;
    }

    // Publish to context so physics.fps.move and other steps use the same dt.
    context.Set<float>("physics_dt", dt);

    // Bullet's stepSimulation interpolates substeps internally at fixedTimeStep
    // (default 1/60) so passing a small dt at 240Hz won't destabilize physics —
    // it just lets Bullet skip a substep when not enough wall time has elapsed.
    world->stepSimulation(dt, maxSubSteps);
}

}  // namespace sdl3cpp::services::impl

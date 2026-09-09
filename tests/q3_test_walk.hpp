#pragma once

// Drives the real pmove chain for one second of forward input against a
// q3test::Scene and reports what happened, so movement tests read as
// "walk into X, expect Y".

#include "q3_test_scene.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow_context.hpp"

namespace q3test {

struct WalkResult {
    float travelledX;  ///< distance covered along +X
    float rise;        ///< height gained
};

/// Start at x = -3 on the floor, face +X, hold forward for one second.
inline WalkResult WalkForward(Scene& scene) {
    namespace impl = sdl3cpp::services::impl;
    sdl3cpp::services::WorkflowContext context;

    impl::Q3PlayerState ps;
    ps.origin = glm::vec3(-3.f, -sdl3cpp::q3::kPlayerFeet, 0.f);
    context.Set("q3.ps", ps);
    context.Set<btDiscreteDynamicsWorld*>("physics_world", &scene.world);
    context.Set<double>("frame.delta_time", 1.0 / 125.0);
    context.Set<float>("q3.player_yaw", -1.5707963f);  // forward is +X
    context.Set<float>("input.move_forward", 1.0f);
    context.Set<float>("input.move_right", 0.0f);

    impl::WorkflowQ3PmGroundStep ground(nullptr);
    impl::WorkflowQ3PmFrictionStep friction(nullptr);
    impl::WorkflowQ3PmAccelerateStep accelerate(nullptr);
    impl::WorkflowQ3PmStepSlideStep slide(nullptr);
    const sdl3cpp::services::WorkflowStepDefinition step;

    const float startY = ps.origin.y;
    for (int i = 0; i < 125; ++i) {
        ground.Execute(step, context);
        friction.Execute(step, context);
        accelerate.Execute(step, context);
        slide.Execute(step, context);
    }
    const auto end = context.Get<impl::Q3PlayerState>(
                                "q3.ps", impl::Q3PlayerState{}).origin;
    return WalkResult{end.x + 3.f, end.y - startY};
}

}  // namespace q3test

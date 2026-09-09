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
/// When `withPlayerBody` is set the scene also contains the player's own
/// capsule, as the game does, so traces must exclude it to move at all.
inline WalkResult WalkForward(Scene& scene, bool withPlayerBody = false) {
    namespace impl = sdl3cpp::services::impl;
    sdl3cpp::services::WorkflowContext context;

    impl::Q3PlayerState ps;
    ps.origin = glm::vec3(-3.f, -sdl3cpp::q3::kPlayerFeet, 0.f);
    context.Set("q3.ps", ps);
    context.Set<btDiscreteDynamicsWorld*>("physics_world", &scene.world);
    btRigidBody* playerBody = nullptr;
    if (withPlayerBody) {
        playerBody = scene.AddPlayerBody(
            btVector3(ps.origin.x, ps.origin.y, ps.origin.z));
        context.Set<std::string>("physics_player_body", "player");
        context.Set<btRigidBody*>("physics_body_player", playerBody);
    }
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

        // q3.player.commit keeps the capsule on the player every frame,
        // so it is always sitting on the next frame's trace origin.
        if (playerBody) {
            const auto now = context.Get<impl::Q3PlayerState>(
                                    "q3.ps", impl::Q3PlayerState{}).origin;
            btTransform t = playerBody->getWorldTransform();
            t.setOrigin(btVector3(now.x, now.y, now.z));
            playerBody->setWorldTransform(t);
            playerBody->getMotionState()->setWorldTransform(t);
        }
    }
    const auto end = context.Get<impl::Q3PlayerState>(
                                "q3.ps", impl::Q3PlayerState{}).origin;
    return WalkResult{end.x + 3.f, end.y - startY};
}

}  // namespace q3test

// Step-up against known geometry: a wall must stay a wall, a step must
// be walkable. The player was reported climbing walls, and a player
// part way up one cannot move in that direction, which looks like the
// direction itself being broken.

#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace impl = sdl3cpp::services::impl;
namespace q3 = sdl3cpp::q3;
using sdl3cpp::services::WorkflowContext;
using sdl3cpp::services::WorkflowStepDefinition;

namespace {

struct Scene {
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher{&config};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver, &config};
    std::vector<std::unique_ptr<btBoxShape>> shapes;
    std::vector<std::unique_ptr<btDefaultMotionState>> motions;
    std::vector<std::unique_ptr<btRigidBody>> bodies;

    // Static box centred at (x,y,z) with the given half extents.
    void Add(btVector3 centre, btVector3 half) {
        shapes.push_back(std::make_unique<btBoxShape>(half));
        motions.push_back(std::make_unique<btDefaultMotionState>(
            btTransform(btQuaternion(0, 0, 0, 1), centre)));
        bodies.push_back(std::make_unique<btRigidBody>(
            btRigidBody::btRigidBodyConstructionInfo(
                0.f, motions.back().get(), shapes.back().get(),
                btVector3(0, 0, 0))));
        world.addRigidBody(bodies.back().get());
    }
};

struct Result {
    float travelledX;
    float rise;
};

// Walk +X for a second and report distance and height gained.
Result WalkInto(Scene& scene) {
    WorkflowContext context;
    impl::Q3PlayerState ps;
    ps.origin = glm::vec3(-3.f, -q3::kPlayerFeet, 0.f);
    context.Set("q3.ps", ps);
    context.Set<btDiscreteDynamicsWorld*>("physics_world", &scene.world);
    context.Set<double>("frame.delta_time", 1.0 / 125.0);
    // yaw such that forward is +X: forward = (-sin, 0, -cos)
    context.Set<float>("q3.player_yaw", -1.5707963f);
    context.Set<float>("input.move_forward", 1.0f);
    context.Set<float>("input.move_right", 0.0f);

    impl::WorkflowQ3PmGroundStep ground(nullptr);
    impl::WorkflowQ3PmFrictionStep friction(nullptr);
    impl::WorkflowQ3PmAccelerateStep accelerate(nullptr);
    impl::WorkflowQ3PmStepSlideStep slide(nullptr);
    const WorkflowStepDefinition step;

    const float startY =
        context.Get<impl::Q3PlayerState>("q3.ps", impl::Q3PlayerState{})
            .origin.y;
    for (int i = 0; i < 125; ++i) {
        ground.Execute(step, context);
        friction.Execute(step, context);
        accelerate.Execute(step, context);
        slide.Execute(step, context);
    }
    const auto end =
        context.Get<impl::Q3PlayerState>("q3.ps", impl::Q3PlayerState{}).origin;
    return Result{end.x + 3.f, end.y - startY};
}

void AddFloor(Scene& scene) {
    scene.Add(btVector3(0, -10.f, 0), btVector3(50.f, 10.f, 50.f));
}

}  // namespace

TEST(StepUp, WalkingIntoAWallDoesNotClimbIt) {
    Scene scene;
    AddFloor(scene);
    // A tall wall at x = 0, far higher than a step.
    scene.Add(btVector3(1.f, 3.f, 0.f), btVector3(1.f, 3.f, 10.f));
    const auto result = WalkInto(scene);
    EXPECT_LT(result.rise, q3::kStepSize)
        << "the player climbed " << result.rise << "m up a wall";
}

TEST(StepUp, WalkingIntoAWallStopsAtIt) {
    Scene scene;
    AddFloor(scene);
    scene.Add(btVector3(1.f, 3.f, 0.f), btVector3(1.f, 3.f, 10.f));
    const auto result = WalkInto(scene);
    EXPECT_LT(result.travelledX, 3.5f) << "the player went through a wall";
}

TEST(StepUp, AKerbIsWalkedOnto) {
    Scene scene;
    AddFloor(scene);
    // A 0.3m step, well inside Quake's 0.5625m step height, extending
    // far enough that a second of running stays on top of it.
    scene.Add(btVector3(26.f, 0.15f, 0.f), btVector3(25.f, 0.15f, 10.f));
    const auto result = WalkInto(scene);
    EXPECT_GT(result.rise, 0.2f)
        << "the player failed to step onto a 0.3m kerb";
}

TEST(StepUp, OpenGroundGainsNoHeight) {
    Scene scene;
    AddFloor(scene);
    const auto result = WalkInto(scene);
    EXPECT_NEAR(result.rise, 0.0f, 0.05f);
    EXPECT_GT(result.travelledX, 5.0f);
}

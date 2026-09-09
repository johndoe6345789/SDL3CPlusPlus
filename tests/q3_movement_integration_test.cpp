// Runs the real pmove step chain against a real Bullet world and
// measures how far the player actually travels. Unit tests proved the
// wish direction is symmetric; this asks whether the player is, which
// is the thing that felt wrong on screen.

#include "services/interfaces/workflow/quake3/workflow_q3_pm_accelerate_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_friction_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_ground_step.hpp"
#include "services/interfaces/workflow/quake3/workflow_q3_pm_step_slide_step.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"
#include "services/interfaces/workflow/quake3/q3_pm_types.hpp"
#include "services/interfaces/workflow_context.hpp"

#include <btBulletDynamicsCommon.h>
#include <gtest/gtest.h>

#include <cmath>
#include <memory>

namespace impl = sdl3cpp::services::impl;
namespace q3 = sdl3cpp::q3;

namespace {

// A flat world: one large static box with its top at y = 0.
struct Arena {
    btDefaultCollisionConfiguration config;
    btCollisionDispatcher dispatcher{&config};
    btDbvtBroadphase broadphase;
    btSequentialImpulseConstraintSolver solver;
    btDiscreteDynamicsWorld world{&dispatcher, &broadphase, &solver, &config};
    btBoxShape ground{btVector3(200.f, 10.f, 200.f)};
    btDefaultMotionState motion{
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, -10.f, 0))};
    btRigidBody body{btRigidBody::btRigidBodyConstructionInfo(
        0.f, &motion, &ground, btVector3(0, 0, 0))};

    Arena() { world.addRigidBody(&body); }
};

// Drive the chain for a fixed time and report horizontal displacement.
float TravelDistance(float moveForward, float moveRight, float yaw) {
    Arena arena;
    sdl3cpp::services::WorkflowContext context;
    const sdl3cpp::services::WorkflowStepDefinition step;

    impl::Q3PlayerState ps;
    ps.origin = glm::vec3(0.f, -q3::kPlayerFeet, 0.f);  // feet on y = 0
    context.Set("q3.ps", ps);
    context.Set<btDiscreteDynamicsWorld*>("physics_world", &arena.world);
    context.Set<double>("frame.delta_time", 1.0 / 125.0);
    context.Set<float>("q3.player_yaw", yaw);
    context.Set<float>("input.move_forward", moveForward);
    context.Set<float>("input.move_right", moveRight);

    impl::WorkflowQ3PmGroundStep ground(nullptr);
    impl::WorkflowQ3PmFrictionStep friction(nullptr);
    impl::WorkflowQ3PmAccelerateStep accelerate(nullptr);
    impl::WorkflowQ3PmStepSlideStep slide(nullptr);

    const glm::vec3 start = context.Get<impl::Q3PlayerState>(
                                   "q3.ps", impl::Q3PlayerState{}).origin;
    for (int frame = 0; frame < 125; ++frame) {  // one second
        ground.Execute(step, context);
        friction.Execute(step, context);
        accelerate.Execute(step, context);
        slide.Execute(step, context);
    }
    const glm::vec3 end = context.Get<impl::Q3PlayerState>(
                                 "q3.ps", impl::Q3PlayerState{}).origin;

    const glm::vec3 moved = end - start;
    return std::sqrt(moved.x * moved.x + moved.z * moved.z);
}

}  // namespace

TEST(Movement, PlayerActuallyMovesOnFlatGround) {
    EXPECT_GT(TravelDistance(1.f, 0.f, 0.f), 1.f)
        << "a second of forward input should cover real ground";
}

TEST(Movement, TravelIsTheSameInEveryFacing) {
    const float reference = TravelDistance(1.f, 0.f, 0.f);
    ASSERT_GT(reference, 1.f);
    for (int step = 0; step < 8; ++step) {
        const float yaw = static_cast<float>(step) * 0.785398f;
        EXPECT_NEAR(TravelDistance(1.f, 0.f, yaw), reference,
                    reference * 0.02f)
            << "forward at yaw " << yaw << " differs";
    }
}

TEST(Movement, TravelIsTheSameForEveryInputDirection) {
    const float reference = TravelDistance(1.f, 0.f, 0.f);
    const struct { float f, r; const char* what; } cases[] = {
        {-1.f, 0.f, "back"}, {0.f, 1.f, "strafe right"},
        {0.f, -1.f, "strafe left"}};
    for (const auto& c : cases) {
        EXPECT_NEAR(TravelDistance(c.f, c.r, 0.f), reference,
                    reference * 0.02f) << c.what;
    }
}

TEST(Movement, DiagonalTravelsNoFurtherThanStraight) {
    const float straight = TravelDistance(1.f, 0.f, 0.f);
    const float diagonal = TravelDistance(1.f, 1.f, 0.f);
    EXPECT_NEAR(diagonal, straight, straight * 0.02f);
}

TEST(Movement, TopSpeedMatchesQuake) {
    // 320 units/second is 10 m/s; a second of running, minus the ramp
    // up, should land close to that.
    const float travelled = TravelDistance(1.f, 0.f, 0.f);
    EXPECT_GT(travelled, q3::kMaxSpeed * 0.8f);
    EXPECT_LT(travelled, q3::kMaxSpeed * 1.05f);
}

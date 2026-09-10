// Climbing a flight of stairs, as opposed to the single kerb
// q3_step_up_test covers. A staircase in a doorway, and one that turns a
// corner, are where the player was reported stopping dead: the box
// overhangs two treads at once there, so what is underfoot is the
// diagonal between them rather than either tread.

#include "q3_test_walk.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"

#include <gtest/gtest.h>

namespace q3 = sdl3cpp::q3;
using q3test::Scene;
using q3test::WalkForward;

namespace {

constexpr float kRise  = 0.4f;  // 12.8 Quake units, inside the step height
constexpr float kDepth = 0.7f;
constexpr int kSteps   = 6;

/// A flight ascending along +X from `firstStepX`. Each step is a slab
/// reaching back under the ones above it, the way a mapper builds them.
void AddStaircase(Scene& scene, float firstStepX) {
    for (int i = 0; i < kSteps; ++i) {
        const float top   = kRise * static_cast<float>(i + 1);
        const float front = firstStepX + kDepth * static_cast<float>(i);
        const float halfX = (firstStepX + 40.f - front) * 0.5f;
        scene.Add(btVector3(front + halfX, top - 10.f, 0.f),
                  btVector3(halfX, 10.f, 6.f));
    }
}

/// Walls either side, as a staircase inside a doorway has.
void AddSideWalls(Scene& scene, float halfWidth) {
    scene.Add(btVector3(10.f, 5.f, halfWidth + 1.f),
              btVector3(30.f, 5.f, 1.f));
    scene.Add(btVector3(10.f, 5.f, -halfWidth - 1.f),
              btVector3(30.f, 5.f, 1.f));
}

}  // namespace

TEST(Stairs, AFlightIsClimbed) {
    Scene scene;
    scene.AddFloor();
    AddStaircase(scene, 1.f);
    const auto result = WalkForward(scene);
    EXPECT_GT(result.rise, kRise * 3.f)
        << "the player climbed only " << result.rise << "m of a "
        << kRise * kSteps << "m flight";
}

TEST(Stairs, AFlightInADoorwayIsClimbed) {
    Scene scene;
    scene.AddFloor();
    AddStaircase(scene, 1.f);
    AddSideWalls(scene, 0.8f);
    const auto result = WalkForward(scene);
    EXPECT_GT(result.rise, kRise * 3.f)
        << "walls either side stopped the climb at " << result.rise << "m";
}

TEST(Stairs, AStepIsClimbedWhenTheBoxOverhangsTwoTreads) {
    // The reported wedge: approach where the leading edge meets the
    // riser of the step above, so the settle trace lands on the corner
    // between the two rather than on a tread.
    Scene scene;
    scene.AddFloor();
    scene.Add(btVector3(21.f, 0.2f - 10.f, 0.f), btVector3(20.f, 10.f, 6.f));
    scene.Add(btVector3(21.7f, 0.6f - 10.f, 0.f), btVector3(20.f, 10.f, 6.f));
    const auto result = WalkForward(scene);
    EXPECT_GT(result.rise, 0.5f)
        << "the player wedged on the corner at " << result.rise << "m";
}

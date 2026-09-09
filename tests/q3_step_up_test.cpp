// Step-up against known geometry: a wall must stay a wall, a step must
// be walkable. The player was reported climbing walls, and a player
// part way up one cannot move in that direction, which looks like the
// direction itself being broken.

#include "q3_test_walk.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"

#include <gtest/gtest.h>

namespace q3 = sdl3cpp::q3;
using q3test::Scene;
using q3test::WalkForward;

namespace {
void AddWall(Scene& scene) {
    scene.Add(btVector3(1.f, 3.f, 0.f), btVector3(1.f, 3.f, 10.f));
}
}  // namespace

TEST(StepUp, WalkingIntoAWallDoesNotClimbIt) {
    Scene scene;
    scene.AddFloor();
    AddWall(scene);
    const auto result = WalkForward(scene);
    EXPECT_LT(result.rise, q3::kStepSize)
        << "the player climbed " << result.rise << "m up a wall";
}

TEST(StepUp, WalkingIntoAWallStopsAtIt) {
    Scene scene;
    scene.AddFloor();
    AddWall(scene);
    EXPECT_LT(WalkForward(scene).travelledX, 3.5f)
        << "the player went through a wall";
}

TEST(StepUp, AKerbIsWalkedOnto) {
    Scene scene;
    scene.AddFloor();
    // A 0.3m step, well inside Quake's 0.5625m step height, extending
    // far enough that a second of running stays on top of it.
    scene.Add(btVector3(26.f, 0.15f, 0.f), btVector3(25.f, 0.15f, 10.f));
    EXPECT_GT(WalkForward(scene).rise, 0.2f)
        << "the player failed to step onto a 0.3m kerb";
}

TEST(StepUp, OpenGroundGainsNoHeight) {
    Scene scene;
    scene.AddFloor();
    const auto result = WalkForward(scene);
    EXPECT_NEAR(result.rise, 0.0f, 0.05f);
    EXPECT_GT(result.travelledX, 5.0f);
}


TEST(StepUp, WalksWithTheirOwnBodyInTheWorld) {
    // The game keeps the player's dynamic capsule in the world and
    // q3.player.commit teleports it onto them every frame. This guards
    // the invariant that its presence must not impede movement.
    //
    // It does not reproduce the fault that setting Q3NotMeCallback::me
    // fixed in game: Bullet's convexSweepTest declines to report a body
    // the sweep starts inside, so this passes with or without that
    // assignment. Kept as a guard, not as that bug's regression test.
    Scene scene;
    scene.AddFloor();
    const auto result = WalkForward(scene, /*withPlayerBody=*/true);
    EXPECT_GT(result.travelledX, 5.0f)
        << "the player was blocked by their own collision body";
}

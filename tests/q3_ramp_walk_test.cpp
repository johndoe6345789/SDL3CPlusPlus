// Walking up a slope. Reported from q3dm1: approaching "the tongue"
// slowly leaves the player stuck against it, while running up the same
// slope works. A ramp is the case a slide move has to climb rather than
// stop against, so it is exercised here on its own geometry.

#include "q3_test_walk.hpp"
#include "services/interfaces/workflow/quake3/q3_slide_move.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <cstdio>

namespace q3 = sdl3cpp::q3;
using q3test::Scene;
using q3test::WalkForward;

namespace {

/// A slope rising along +X, its foot at about x = 0 and its top face
/// passing through the floor plane there, so a player walking from -X
/// meets the surface rather than a lip.
void AddTongue(Scene& scene, float degrees) {
    const float pitch = degrees * 3.14159265f / 180.f;
    const float c = std::cos(pitch);
    const float sn = std::sin(pitch);
    // Thick enough that the player never reaches its underside, long
    // enough that a second of walking stays on it.
    const float halfLength = 6.f;
    const float halfDepth = 1.f;
    const btVector3 half(halfLength, halfDepth, 5.f);
    // Place the box so its top face lies in the plane through the
    // origin with normal (-sin, cos, 0): the slope then meets the floor
    // flush at x = 0 for every angle, with no lip to trip over.
    const float cx = halfDepth * sn + halfLength * c;
    const float cy = -halfDepth * c + halfLength * sn;
    scene.AddRamp(btVector3(cx, cy, 0.f), half, pitch);
}

}  // namespace

// A slope the player can stand on must be climbable at any speed. The
// reported fault was speed dependent: running up the tongue on q3dm1
// worked, walking up it left the player stuck against the foot.
TEST(RampWalk, RunningUpAWalkableSlopeClimbsIt) {
    Scene scene;
    scene.AddFloor();
    AddTongue(scene, 35.f);
    const auto result = WalkForward(scene, /*withPlayerBody=*/false, 1.0f);
    EXPECT_GT(result.rise, 1.0f)
        << "running up a 35 degree slope rose only " << result.rise << "m";
}

TEST(RampWalk, WalkingUpAWalkableSlopeClimbsIt) {
    Scene scene;
    scene.AddFloor();
    AddTongue(scene, 35.f);
    // Half input: Quake's walk button, not a crawl.
    const auto result = WalkForward(scene, /*withPlayerBody=*/false, 0.5f);
    EXPECT_GT(result.rise, 0.2f)
        << "walking up a 35 degree slope rose only " << result.rise
        << "m — the player stuck at the foot of the slope";
}

TEST(RampWalk, WalkingUpASlopeGetsPastItsFoot) {
    Scene scene;
    scene.AddFloor();
    AddTongue(scene, 35.f);
    // The slope starts at x = 0 and the player starts 3m short of it, so
    // anything under about 2.9m never left the flat.
    const auto result = WalkForward(scene, /*withPlayerBody=*/false, 0.5f);
    EXPECT_GT(result.travelledX, 2.9f)
        << "walking, the player advanced " << result.travelledX
        << "m and stopped against the foot of the slope";
}

// The other half of the contract: stepping must not become a way to
// climb terrain Quake considers too steep to stand on.
TEST(RampWalk, ASlopeTooSteepToStandOnIsNotClimbed) {
    Scene scene;
    scene.AddFloor();
    AddTongue(scene, 60.f);  // MIN_WALK_NORMAL allows about 45.6 degrees
    const auto running = WalkForward(scene, /*withPlayerBody=*/false, 1.0f);
    EXPECT_LT(running.rise, 0.3f)
        << "the player climbed " << running.rise
        << "m up a 60 degree slope";
}

TEST(RampWalk, AWalkableSlopeIsClimbedWithTheirOwnBodyInTheWorld) {
    Scene scene;
    scene.AddFloor();
    AddTongue(scene, 35.f);
    const auto result = WalkForward(scene, /*withPlayerBody=*/true, 0.5f);
    EXPECT_GT(result.rise, 0.2f)
        << "the player was blocked by their own collision body";
}

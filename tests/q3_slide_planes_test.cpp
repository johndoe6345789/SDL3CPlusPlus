// Velocity resolution against clip planes, per ioq3 bg_slidemove.c.
// The player getting stuck on a flat wall was this logic cancelling a
// move that should have slid, so each case below is a movement bug.

#include "services/interfaces/workflow/quake3/q3_slide_planes.hpp"

#include <gtest/gtest.h>

using sdl3cpp::q3::ClipVelocity;
using sdl3cpp::q3::ResolveAgainstPlanes;

namespace {

constexpr float kTol = 1e-4f;

const glm::vec3 kWallEast{-1.0f, 0.0f, 0.0f};   // faces -X
const glm::vec3 kWallNorth{0.0f, 0.0f, -1.0f};  // faces -Z
const glm::vec3 kFloor{0.0f, 1.0f, 0.0f};

}  // namespace

TEST(ClipVelocity, RemovesTheComponentIntoTheSurface) {
    const glm::vec3 out = ClipVelocity({1.0f, 0.0f, 0.0f}, kWallEast, 1.0f);
    EXPECT_NEAR(out.x, 0.0f, kTol);
}

TEST(ClipVelocity, OverbouncePushesBackOutOfTheSurface) {
    const glm::vec3 out = ClipVelocity({1.0f, 0.0f, 0.0f}, kWallEast, 1.001f);
    EXPECT_LT(out.x, 0.0f) << "overbounce should leave the surface";
}

TEST(ClipVelocity, LeavesMotionAlongTheSurfaceUntouched) {
    const glm::vec3 out = ClipVelocity({0.0f, 0.0f, 5.0f}, kWallEast, 1.001f);
    EXPECT_NEAR(out.z, 5.0f, kTol);
}

TEST(ResolveAgainstPlanes, PassesThroughWhenNoPlaneIsEntered) {
    const glm::vec3 velocity{0.0f, 0.0f, 5.0f};
    const glm::vec3 out = ResolveAgainstPlanes(velocity, &kWallEast, 1);
    EXPECT_NEAR(out.z, 5.0f, kTol);
}

TEST(ResolveAgainstPlanes, SlidesAlongASingleWall) {
    // Walking diagonally into a wall must keep the tangential component:
    // zeroing it here is what pinned the player against flat surfaces.
    const glm::vec3 out =
        ResolveAgainstPlanes({5.0f, 0.0f, 5.0f}, &kWallEast, 1);
    EXPECT_GT(out.z, 4.0f);
    EXPECT_LE(out.x, 0.0f);
}

TEST(ResolveAgainstPlanes, SlidesAlongTheCreaseOfTwoWalls) {
    const glm::vec3 planes[] = {kWallEast, kWallNorth};
    const glm::vec3 out = ResolveAgainstPlanes({5.0f, 3.0f, 5.0f}, planes, 2);
    EXPECT_NEAR(out.x, 0.0f, 0.5f);
    EXPECT_NEAR(out.z, 0.0f, 0.5f);
    EXPECT_GT(std::abs(out.y), 0.5f) << "should slide up the crease";
}

TEST(ResolveAgainstPlanes, AnAxialCornerBleedsOffRatherThanStopping) {
    // Clipping against the second plane stops the move re-entering the
    // first, so ioq3 never reaches the crease and leaves a residue
    // instead of killing the velocity. Documented because it looks like
    // it ought to stop dead and does not.
    const glm::vec3 planes[] = {kWallEast, kWallNorth, kFloor};
    const glm::vec3 out =
        ResolveAgainstPlanes({5.0f, -3.0f, 5.0f}, planes, 3);
    EXPECT_LT(glm::length(out), 0.05f);
}

TEST(ResolveAgainstPlanes, StopsDeadWhenAThirdPlaneBlocksTheCrease) {
    // A wedge where clipping against the second plane pushes back into
    // the first, so the crease is taken, and a third plane still blocks
    // it. This is the only path that zeroes velocity.
    const glm::vec3 planes[] = {{-0.1744f, -0.9846f, -0.0102f},
                                {-0.3150f, 0.8281f, 0.4638f},
                                {-0.0322f, 0.8909f, -0.4530f}};
    const glm::vec3 out =
        ResolveAgainstPlanes({6.3128f, -0.0324f, 3.5003f}, planes, 3);
    EXPECT_NEAR(glm::length(out), 0.0f, kTol);
}

TEST(ResolveAgainstPlanes, TreatsGrazingContactAsNonBlocking) {
    // Below the 0.1 epsilon the move is considered parallel, so floating
    // point noise on a surface being slid along is not a fresh collision.
    const glm::vec3 velocity{0.05f, 0.0f, 5.0f};
    const glm::vec3 out = ResolveAgainstPlanes(velocity, &kWallEast, 1);
    EXPECT_NEAR(out.z, 5.0f, kTol);
}

TEST(ResolveAgainstPlanes, NoPlanesLeavesVelocityUnchanged) {
    const glm::vec3 velocity{1.0f, 2.0f, 3.0f};
    const glm::vec3 out = ResolveAgainstPlanes(velocity, nullptr, 0);
    EXPECT_NEAR(glm::length(out - velocity), 0.0f, kTol);
}

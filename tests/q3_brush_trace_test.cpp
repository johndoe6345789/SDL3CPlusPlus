// Box-vs-brush tracing against geometry whose answer is known by hand.
// This is the property Bullet's convex sweep could not give us: the
// exact plane that stopped the move, and an honest "started solid".

#include "services/interfaces/workflow/quake3/q3_brush_trace.hpp"

#include <gtest/gtest.h>

namespace impl = sdl3cpp::services::impl;

namespace {

/// An axis-aligned box brush, with the six axial planes q3map2 emits.
void AddBox(impl::BrushCollisionModel& model, glm::vec3 mins,
            glm::vec3 maxs) {
    impl::CollisionBrush brush;
    brush.firstSide = static_cast<int>(model.sides.size());
    brush.numSides  = 6;
    brush.mins      = mins;
    brush.maxs      = maxs;
    for (int axis = 0; axis < 3; ++axis) {
        glm::vec3 positive(0.f);
        positive[axis] = 1.f;
        model.sides.push_back({positive, maxs[axis]});
        glm::vec3 negative(0.f);
        negative[axis] = -1.f;
        model.sides.push_back({negative, -mins[axis]});
    }
    model.brushes.push_back(brush);
}

/// A unit-ish player box, as the pmove one is: taller than it is wide
/// and not centred on the origin.
constexpr glm::vec3 kMins(-0.5f, -0.75f, -0.5f);
constexpr glm::vec3 kMaxs(0.5f, 1.0f, 0.5f);

impl::BrushCollisionModel FloorModel() {
    impl::BrushCollisionModel model;
    AddBox(model, glm::vec3(-50.f, -10.f, -50.f), glm::vec3(50.f, 0.f, 50.f));
    return model;
}

}  // namespace

TEST(BrushTrace, AnUnobstructedSweepReachesItsTarget) {
    const auto model = FloorModel();
    const auto trace = impl::TraceBoxThroughBrushes(
        model, glm::vec3(0.f, 5.f, 0.f), glm::vec3(4.f, 5.f, 0.f), kMins,
        kMaxs);
    EXPECT_FALSE(trace.hit);
    EXPECT_FLOAT_EQ(trace.fraction, 1.f);
    EXPECT_FALSE(trace.startSolid);
}

TEST(BrushTrace, LandingOnTheFloorReportsTheFloorPlane) {
    const auto model = FloorModel();
    // Feet start 2m up; the box's mins reach the floor after 2m of fall.
    const auto trace = impl::TraceBoxThroughBrushes(
        model, glm::vec3(0.f, 2.75f, 0.f), glm::vec3(0.f, -1.25f, 0.f), kMins,
        kMaxs);
    ASSERT_TRUE(trace.hit);
    EXPECT_NEAR(trace.normal.y, 1.f, 1e-4f) << "expected the floor's up face";
    EXPECT_NEAR(trace.endPos.y, 0.75f, 0.01f)
        << "the box should come to rest with its feet on the floor";
}

TEST(BrushTrace, WalkingIntoAWallReportsTheWallPlane) {
    auto model = FloorModel();
    AddBox(model, glm::vec3(2.f, 0.f, -5.f), glm::vec3(3.f, 4.f, 5.f));
    const auto trace = impl::TraceBoxThroughBrushes(
        model, glm::vec3(0.f, 0.75f, 0.f), glm::vec3(4.f, 0.75f, 0.f), kMins,
        kMaxs);
    ASSERT_TRUE(trace.hit);
    EXPECT_NEAR(trace.normal.x, -1.f, 1e-4f) << "expected the wall's -X face";
    EXPECT_LT(trace.endPos.x, 1.51f) << "the box overran the wall";
}

TEST(BrushTrace, ADownwardSweepOnAStepReportsItsTreadNotItsSide) {
    // The case Bullet got wrong: probing straight down while the box
    // overhangs a step's edge must report the tread, not the riser.
    auto model = FloorModel();
    AddBox(model, glm::vec3(1.f, 0.f, -5.f), glm::vec3(9.f, 0.4f, 5.f));
    const auto trace = impl::TraceBoxThroughBrushes(
        model, glm::vec3(1.4f, 1.15f, 0.f), glm::vec3(1.4f, 0.95f, 0.f), kMins,
        kMaxs);
    ASSERT_TRUE(trace.hit);
    EXPECT_NEAR(trace.normal.y, 1.f, 1e-4f)
        << "a downward probe reported a normal of (" << trace.normal.x << ", "
        << trace.normal.y << ", " << trace.normal.z << ")";
}

TEST(BrushTrace, StartingInsideABrushIsReportedAsSolid) {
    const auto model = FloorModel();
    const auto trace = impl::TraceBoxThroughBrushes(
        model, glm::vec3(0.f, -5.f, 0.f), glm::vec3(0.f, -4.f, 0.f), kMins,
        kMaxs);
    EXPECT_TRUE(trace.startSolid)
        << "a sweep begun inside the floor must say so";
}

TEST(BrushTrace, PlayerClipStopsMovementButNotShots) {
    impl::BrushCollisionModel model;
    AddBox(model, glm::vec3(2.f, -1.f, -5.f), glm::vec3(3.f, 4.f, 5.f));
    model.brushes.back().playerClip = true;

    const auto move = impl::TraceBoxThroughBrushes(
        model, glm::vec3(0.f, 0.75f, 0.f), glm::vec3(4.f, 0.75f, 0.f), kMins,
        kMaxs, /*includePlayerClip=*/true);
    EXPECT_TRUE(move.hit) << "pmove should be stopped by a clip brush";

    const auto shot = impl::TraceBoxThroughBrushes(
        model, glm::vec3(0.f, 0.75f, 0.f), glm::vec3(4.f, 0.75f, 0.f), kMins,
        kMaxs, /*includePlayerClip=*/false);
    EXPECT_FALSE(shot.hit) << "a shot should pass through a clip brush";
}

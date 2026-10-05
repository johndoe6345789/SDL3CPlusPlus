#include "services/interfaces/workflow/racer/world/racer_ground.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

TEST(RacerGround, FindsHeightOnASlope) {
    RacerGround ground;
    AddRacerGroundTriangle(ground, {0, 0, 0}, {20, 10, 0}, {0, 0, 20});
    const auto h = RacerGroundHeight(ground, 10.f, 1.f, 100.f);
    ASSERT_TRUE(h.has_value());
    EXPECT_NEAR(*h, 5.f, 0.01f);
}

TEST(RacerGround, PicksHighestSurfaceUnderTheCeiling) {
    RacerGround ground;
    AddRacerGroundTriangle(ground, {0, 0, 0}, {20, 0, 0}, {0, 0, 20});
    AddRacerGroundTriangle(ground, {0, 30, 0}, {20, 30, 0}, {0, 30, 20});
    EXPECT_NEAR(*RacerGroundHeight(ground, 2.f, 2.f, 10.f), 0.f, 1e-4f);
    EXPECT_NEAR(*RacerGroundHeight(ground, 2.f, 2.f, 40.f), 30.f, 1e-4f);
}

TEST(RacerGround, IgnoresWalls) {
    RacerGround ground;
    AddRacerGroundTriangle(ground, {0, 0, 0}, {0, 20, 0}, {0, 0, 20});
    EXPECT_TRUE(ground.triangles.empty());
}

TEST(RacerGround, EmptyOutsideAnyTriangle) {
    RacerGround ground;
    AddRacerGroundTriangle(ground, {0, 0, 0}, {20, 0, 0}, {0, 0, 20});
    EXPECT_FALSE(RacerGroundHeight(ground, 500.f, 500.f, 10.f).has_value());
}

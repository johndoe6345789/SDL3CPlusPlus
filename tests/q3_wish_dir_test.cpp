// Movement input to a world-space wish. Speed must not depend on the
// direction pressed: using the input vector's magnitude makes diagonals
// sqrt(2) faster, which feels like the straight directions are stuck.

#include "services/interfaces/workflow/quake3/q3_wish_dir.hpp"

#include <gtest/gtest.h>
#include <cmath>

namespace q3 = sdl3cpp::q3;

namespace {
constexpr float kSpeed = 10.0f;
constexpr float kTol = 1e-4f;
}  // namespace

TEST(ComputeWish, ForwardAtZeroYawGoesDownNegativeZ) {
    const auto wish = q3::ComputeWish(1.0f, 0.0f, 0.0f, kSpeed);
    EXPECT_NEAR(wish.direction.x, 0.0f, kTol);
    EXPECT_NEAR(wish.direction.z, -1.0f, kTol);
}

TEST(ComputeWish, RightAtZeroYawGoesDownPositiveX) {
    const auto wish = q3::ComputeWish(0.0f, 1.0f, 0.0f, kSpeed);
    EXPECT_NEAR(wish.direction.x, 1.0f, kTol);
    EXPECT_NEAR(wish.direction.z, 0.0f, kTol);
}

TEST(ComputeWish, BackAndLeftAreExactOpposites) {
    const auto fwd = q3::ComputeWish(1.0f, 0.0f, 0.7f, kSpeed);
    const auto back = q3::ComputeWish(-1.0f, 0.0f, 0.7f, kSpeed);
    EXPECT_NEAR(fwd.direction.x, -back.direction.x, kTol);
    EXPECT_NEAR(fwd.direction.z, -back.direction.z, kTol);
    const auto right = q3::ComputeWish(0.0f, 1.0f, 0.7f, kSpeed);
    const auto left = q3::ComputeWish(0.0f, -1.0f, 0.7f, kSpeed);
    EXPECT_NEAR(right.direction.x, -left.direction.x, kTol);
    EXPECT_NEAR(right.direction.z, -left.direction.z, kTol);
}

TEST(ComputeWish, EverySingleAxisPressGivesFullSpeed) {
    for (const auto& in : {std::pair{1.0f, 0.0f}, {-1.0f, 0.0f},
                           {0.0f, 1.0f}, {0.0f, -1.0f}}) {
        const auto wish = q3::ComputeWish(in.first, in.second, 1.2f, kSpeed);
        EXPECT_NEAR(wish.speed, kSpeed, kTol)
            << "fwd=" << in.first << " right=" << in.second;
    }
}

TEST(ComputeWish, DiagonalIsNoFasterThanStraight) {
    // The regression: magnitude-based speed made this sqrt(2) x faster.
    const auto straight = q3::ComputeWish(1.0f, 0.0f, 0.0f, kSpeed);
    const auto diagonal = q3::ComputeWish(1.0f, 1.0f, 0.0f, kSpeed);
    EXPECT_NEAR(diagonal.speed, straight.speed, kTol);
}

TEST(ComputeWish, SpeedIsIndependentOfFacing) {
    float previous = -1.0f;
    for (float yaw = 0.0f; yaw < 6.28f; yaw += 0.4f) {
        const auto wish = q3::ComputeWish(1.0f, 0.0f, yaw, kSpeed);
        if (previous >= 0.0f) EXPECT_NEAR(wish.speed, previous, kTol);
        previous = wish.speed;
        EXPECT_NEAR(glm::length(wish.direction), 1.0f, kTol);
    }
}

TEST(ComputeWish, PartialInputScalesSpeedDown) {
    const auto wish = q3::ComputeWish(0.5f, 0.0f, 0.0f, kSpeed);
    EXPECT_NEAR(wish.speed, kSpeed * 0.5f, kTol);
}

TEST(ComputeWish, NoInputGivesNoWish) {
    const auto wish = q3::ComputeWish(0.0f, 0.0f, 1.0f, kSpeed);
    EXPECT_NEAR(wish.speed, 0.0f, kTol);
    EXPECT_NEAR(glm::length(wish.direction), 0.0f, kTol);
}

TEST(ComputeWish, DirectionRotatesWithYaw) {
    const auto atZero = q3::ComputeWish(1.0f, 0.0f, 0.0f, kSpeed);
    const auto atQuarter =
        q3::ComputeWish(1.0f, 0.0f, static_cast<float>(M_PI_2), kSpeed);
    EXPECT_NEAR(atQuarter.direction.x, -1.0f, kTol);
    EXPECT_NEAR(atZero.direction.z, -1.0f, kTol);
}

// Movement constants against the ioq3 originals. Speeds and distances
// carry a length and scale with the map; per-second coefficients do not.
// Scaling the coefficients is what made movement feel wrong.

#include "services/interfaces/workflow/quake3/q3_pm_constants.hpp"

#include <gtest/gtest.h>

namespace q3 = sdl3cpp::q3;

namespace {
constexpr float kTol = 1e-5f;
// bg_pmove.c / bg_local.h / g_main.c
constexpr float kIoq3Speed = 320.0f;
constexpr float kIoq3StopSpeed = 100.0f;
constexpr float kIoq3Jump = 270.0f;
constexpr float kIoq3Gravity = 800.0f;
}  // namespace

TEST(PmConstants, OneEngineUnitIsThirtyTwoQuakeUnits) {
    EXPECT_NEAR(q3::kQuakeUnitsPerEngineUnit, 32.0f, kTol);
    EXPECT_NEAR(q3::FromQuakeUnits(32.0f), 1.0f, kTol);
}

TEST(PmConstants, MaxSpeedMatchesGSpeed) {
    EXPECT_NEAR(q3::kMaxSpeed, kIoq3Speed / 32.0f, kTol);
    EXPECT_NEAR(q3::kMaxSpeed, 10.0f, kTol);
}

TEST(PmConstants, StopSpeedMatchesPmStopspeed) {
    EXPECT_NEAR(q3::kStopSpeed, kIoq3StopSpeed / 32.0f, kTol);
}

TEST(PmConstants, JumpVelocityMatchesJumpVelocity) {
    EXPECT_NEAR(q3::kJumpVelocity, kIoq3Jump / 32.0f, kTol);
}

TEST(PmConstants, GravityMatchesGGravity) {
    EXPECT_NEAR(q3::kGravity, kIoq3Gravity / 32.0f, kTol);
    EXPECT_NEAR(q3::kGravity, 25.0f, kTol);
}

TEST(PmConstants, RateCoefficientsAreNotScaled) {
    // These are per-second, dimensionless in length, so they are used
    // exactly as ioq3 states them.
    EXPECT_NEAR(q3::kFriction, 6.0f, kTol);
    EXPECT_NEAR(q3::kAccelerate, 10.0f, kTol);
    EXPECT_NEAR(q3::kAirAccelerate, 1.0f, kTol);
    EXPECT_NEAR(q3::kDuckScale, 0.25f, kTol);
}

TEST(PmConstants, GroundAcceleratesTenTimesFasterThanAir) {
    EXPECT_NEAR(q3::kAccelerate / q3::kAirAccelerate, 10.0f, kTol);
}

TEST(PmConstants, JumpApexMatchesQuake) {
    // v^2 / 2g, and 270^2 / 1600 is a hair over 45 Quake units.
    const float apex =
        (q3::kJumpVelocity * q3::kJumpVelocity) / (2.0f * q3::kGravity);
    EXPECT_NEAR(apex * 32.0f, (kIoq3Jump * kIoq3Jump) / (2 * kIoq3Gravity),
                1e-2f);
}

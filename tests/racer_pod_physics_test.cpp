#include "racer_pod_physics_fixture.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;
using namespace racer_test;


TEST(RacerPodPhysics, FullThrottleReachesTopSpeedAndHovers) {
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodState pod = Fly(input, 10.f, Surface(&Flat));
    EXPECT_NEAR(pod.speed, RacerPodSpec{}.topSpeed, 1.f);
    EXPECT_NEAR(pod.position.y, RacerPodSpec{}.hoverHeight, 0.01f);
    EXPECT_TRUE(pod.grounded);
    EXPECT_LT(pod.position.z, -100.f);  // heading 0 flies toward -z
}

TEST(RacerPodPhysics, BoostOverheatsAndDamagesEngines) {
    RacerPodInput input;
    input.throttle = 1.f;
    input.boost = true;
    const RacerPodState pod = Fly(input, 12.f, Surface(&Flat));
    EXPECT_GT(pod.damage, 0.f);
}

TEST(RacerPodPhysics, BoostIsFasterThanTopSpeedBeforeOverheating) {
    RacerPodInput input;
    input.throttle = 1.f;
    RacerPodState pod = Fly(input, 8.f, Surface(&Flat));
    input.boost = true;
    const RacerPodSpec spec;
    for (int i = 0; i < 60; ++i) {
        StepRacerPod(pod, input, spec, 1.f / 60.f, Surface(&Flat));
    }
    EXPECT_GT(pod.speed, spec.topSpeed);
}

TEST(RacerPodPhysics, WallStopsThePod) {
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodState pod = Fly(input, 5.f, Surface(&Walled));
    EXPECT_GT(pod.position.z, -50.f);
}

TEST(RacerPodPhysics, PodFallsOverAGap) {
    RacerPodInput input;
    const RacerPodState pod = Fly(input, 1.f, Surface(&Gap));
    EXPECT_FALSE(pod.grounded);
    EXPECT_LT(pod.position.y, -5.f);
    EXPECT_GT(pod.airTime, 0.9f);
}

TEST(RacerPodPhysics, WallTriangleStopsThePod) {
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodState pod = Fly(input, 5.f, Surface(&Flat, &WallAt30));
    EXPECT_GT(pod.position.z, -30.f);
}

TEST(RacerPodPhysics, SpeedStripsRaiseAndSandLowersTopSpeed) {
    RacerPodState pod;
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodSpec spec;
    pod.surface = 0x4;  // Fast
    EXPECT_GT(RacerPodTargetSpeed(pod, input, spec), spec.topSpeed);
    pod.surface = 0x8;  // Slow
    EXPECT_LT(RacerPodTargetSpeed(pod, input, spec), spec.topSpeed);
}

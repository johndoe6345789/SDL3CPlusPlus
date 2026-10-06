#include "racer_pod_physics_fixture.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;
using namespace racer_test;

TEST(RacerPodSurface, SpeedStripsRaiseAndSandLowersTopSpeed) {
    RacerPodState pod;
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodSpec spec;
    pod.surface = 0x4;  // Fast
    EXPECT_GT(RacerPodTargetSpeed(pod, input, spec), spec.topSpeed);
    pod.surface = 0x8;  // Slow
    EXPECT_LT(RacerPodTargetSpeed(pod, input, spec), spec.topSpeed);
}

TEST(RacerPodSurface, DamagedLeftEnginePullsLeft) {
    RacerPodInput input;
    input.throttle = 1.f;
    RacerPodState pod;
    pod.speed = 100.f;
    pod.engineDamage = {0.8f, 0.f};
    const RacerPodSpec spec;
    for (int i = 0; i < 60; ++i) {
        StepRacerPod(pod, input, spec, 1.f / 60.f, Surface(&Flat));
    }
    EXPECT_LT(pod.heading, -0.1f);
}

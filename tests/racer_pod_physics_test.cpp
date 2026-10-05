#include "services/interfaces/workflow/racer/player/racer_pod_physics.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace {

/// A flat floor at height 0 everywhere, below any ceiling above it.
std::optional<float> Flat(const void*, float, float, float ceiling) {
    return ceiling >= 0.f ? std::optional<float>(0.f) : std::nullopt;
}

/// A floor only where z > -50: beyond that, a wall up to height 5.
std::optional<float> Walled(const void*, float, float z, float ceiling) {
    if (z > -50.f) return ceiling >= 0.f ? std::optional<float>(0.f)
                                         : std::nullopt;
    return ceiling >= 5.f ? std::optional<float>(5.f) : std::nullopt;
}

std::optional<float> Gap(const void*, float, float, float) {
    return std::nullopt;
}

RacerPodState Fly(RacerPodInput input, float seconds, RacerGroundProbe p) {
    RacerPodState pod;
    const RacerPodSpec spec;
    for (float t = 0.f; t < seconds; t += 1.f / 60.f) {
        StepRacerPod(pod, input, spec, 1.f / 60.f, p, nullptr);
    }
    return pod;
}

}  // namespace

TEST(RacerPodPhysics, FullThrottleReachesTopSpeedAndHovers) {
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodState pod = Fly(input, 10.f, &Flat);
    EXPECT_NEAR(pod.speed, RacerPodSpec{}.topSpeed, 1.f);
    EXPECT_NEAR(pod.position.y, RacerPodSpec{}.hoverHeight, 0.01f);
    EXPECT_TRUE(pod.grounded);
    EXPECT_LT(pod.position.z, -100.f);  // heading 0 flies toward -z
}

TEST(RacerPodPhysics, BoostOverheatsAndDamagesEngines) {
    RacerPodInput input;
    input.throttle = 1.f;
    input.boost = true;
    const RacerPodState pod = Fly(input, 12.f, &Flat);
    EXPECT_GT(pod.damage, 0.f);
}

TEST(RacerPodPhysics, BoostIsFasterThanTopSpeedBeforeOverheating) {
    RacerPodInput input;
    input.throttle = 1.f;
    RacerPodState pod = Fly(input, 8.f, &Flat);
    input.boost = true;
    const RacerPodSpec spec;
    for (int i = 0; i < 60; ++i) {
        StepRacerPod(pod, input, spec, 1.f / 60.f, &Flat, nullptr);
    }
    EXPECT_GT(pod.speed, spec.topSpeed);
}

TEST(RacerPodPhysics, WallStopsThePod) {
    RacerPodInput input;
    input.throttle = 1.f;
    const RacerPodState pod = Fly(input, 5.f, &Walled);
    EXPECT_GT(pod.position.z, -50.f);
}

TEST(RacerPodPhysics, PodFallsOverAGap) {
    RacerPodInput input;
    const RacerPodState pod = Fly(input, 1.f, &Gap);
    EXPECT_FALSE(pod.grounded);
    EXPECT_LT(pod.position.y, -5.f);
    EXPECT_GT(pod.airTime, 0.9f);
}

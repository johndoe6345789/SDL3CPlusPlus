#include "services/interfaces/workflow/racer/player/racer_hazards.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace {

std::vector<glm::vec3> StraightLap() {
    std::vector<glm::vec3> lap;
    for (int i = 0; i < 100; ++i) lap.push_back({0.f, 0.f, -10.f * i});
    return lap;
}

}  // namespace

TEST(RacerHazards, EachPlanetGetsItsOwn) {
    const auto lap = StraightLap();
    const auto tatooine = PlaceRacerHazards("Tatooine", lap);
    ASSERT_EQ(tatooine.size(), 3u);
    EXPECT_EQ(tatooine[0].kind, RacerHazardKind::Blaster);
    EXPECT_GT(tatooine[0].at.y, 10.f);  // up on the canyon side
    EXPECT_TRUE(PlaceRacerHazards("Aquilaris", lap).empty());
}

TEST(RacerHazards, AnEruptionHeatsAndLiftsPodsOverTheVent) {
    RacerHazard vent;
    vent.kind = RacerHazardKind::Eruption;
    vent.period = 7.f;
    RacerPodState pod;
    std::vector<RacerHazard> hazards{vent};
    UpdateRacerHazards(hazards, {&pod}, 0.1f);
    EXPECT_GT(pod.heat, 0.f);
    EXPECT_FLOAT_EQ(pod.verticalSpeed, 14.f);
}

TEST(RacerHazards, ALandingRockBattersThePodUnderIt) {
    RacerHazard rock;
    rock.kind = RacerHazardKind::Rockfall;
    rock.period = 9.f;
    rock.clock = 1.5f;
    RacerPodState under, clear;
    under.speed = clear.speed = 100.f;
    clear.position = {50.f, 0.f, 0.f};
    std::vector<RacerHazard> hazards{rock};
    const auto sounds = UpdateRacerHazards(hazards, {&under, &clear}, 0.2f);
    EXPECT_TRUE(sounds & kRacerSoundRock);
    EXPECT_FLOAT_EQ(under.speed, 50.f);
    EXPECT_GT(under.engineDamage[0], 0.f);
    EXPECT_FLOAT_EQ(clear.speed, 100.f);
}

TEST(RacerHazards, TuskensOnlyFireAtPodsInRange) {
    RacerHazard tusken;
    tusken.kind = RacerHazardKind::Blaster;
    tusken.radius = 100.f;
    tusken.period = 1.f;
    tusken.clock = 0.95f;
    RacerPodState far;
    far.position = {500.f, 0.f, 0.f};
    std::vector<RacerHazard> hazards{tusken};
    EXPECT_EQ(UpdateRacerHazards(hazards, {&far}, 0.1f), 0u);
    far.position = {20.f, 0.f, 0.f};
    hazards[0].clock = 0.95f;
    EXPECT_TRUE(UpdateRacerHazards(hazards, {&far}, 0.1f) &
                kRacerSoundBlaster);
}

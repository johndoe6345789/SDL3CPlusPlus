#include "services/interfaces/workflow/racer/player/racer_lap_progress.hpp"
#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace {

/// Drives a race through `laps` full trips round an 8-point lap.
RacerRaceState Lapped(int laps) {
    RacerRaceState race;
    race.countdown = 0.f;
    for (int lap = 0; lap < laps; ++lap) {
        for (int point = 0; point < 8; ++point) {
            AdvanceRacerRace(race, point, 8, 1.f);
        }
    }
    AdvanceRacerRace(race, 0, 8, 1.f);
    return race;
}

}  // namespace

TEST(RacerLap, CountsALapWhenThePodWrapsRound) {
    const RacerRaceState race = Lapped(1);
    EXPECT_EQ(race.lap, 2);
    // Eight one-second steps round the lap, plus the step back at 0
    // that completes it.
    EXPECT_FLOAT_EQ(race.bestLap, 9.f);
}

TEST(RacerLap, FinishesAfterTheLastLap) {
    EXPECT_TRUE(Lapped(3).finished);
    EXPECT_FALSE(Lapped(2).finished);
}

TEST(RacerLap, CountdownHoldsTheClock) {
    RacerRaceState race;
    AdvanceRacerRace(race, 0, 8, 1.f);
    EXPECT_FLOAT_EQ(race.raceTime, 0.f);
    EXPECT_FLOAT_EQ(race.countdown, 2.f);
}

TEST(RacerLap, NearestPointStaysOnTheLocalSection) {
    std::vector<glm::vec3> points;
    for (int i = 0; i < 40; ++i) points.push_back({i * 10.f, 0.f, 0.f});
    points[30] = {51.f, 0.f, 0.f};  // a far section passing close by
    EXPECT_EQ(NearestRacerLapPoint(points, {50.f, 0, 0}, 5), 5);
    EXPECT_EQ(NearestRacerLapPoint(points, {50.f, 0, 0}, -1), 5);
}

TEST(RacerHud, ShowsLapTimeSpeedAndHeat) {
    RacerRaceState race;
    race.countdown = 0.f;
    race.raceTime = 75.5f;
    RacerPodState pod;
    pod.speed = 100.f;
    pod.heat = 0.5f;
    const std::string hud = FormatRacerHud(race, pod);
    EXPECT_NE(hud.find("LAP 1/3"), std::string::npos);
    EXPECT_NE(hud.find("1:15.50"), std::string::npos);
    EXPECT_NE(hud.find("360 KM/H"), std::string::npos);
    EXPECT_NE(hud.find("[#####-----]"), std::string::npos);
}

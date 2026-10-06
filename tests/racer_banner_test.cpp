#include "services/interfaces/workflow/racer/render/racer_hud_step.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

TEST(RacerHud, BannerCountsDownThenGoesThenFinalLap) {
    RacerRaceState race;
    race.countdown = 2.4f;
    EXPECT_EQ(FormatRacerBanner(race), "3");
    race.countdown = 0.f;
    race.raceTime = 0.5f;
    EXPECT_EQ(FormatRacerBanner(race), "GO!");
    race.raceTime = 100.f;
    race.lap = 3;
    race.lapTime = 1.f;
    EXPECT_EQ(FormatRacerBanner(race), "FINAL LAP");
    race.lapTime = 10.f;
    EXPECT_EQ(FormatRacerBanner(race), "");
}

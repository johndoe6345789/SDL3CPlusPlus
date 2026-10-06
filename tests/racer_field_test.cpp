#include "services/interfaces/workflow/racer/player/racer_field_rules.hpp"
#include "services/interfaces/workflow/racer/player/racer_traffic.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

TEST(RacerField, RanksByLapThenPoint) {
    RacerWorldState state;
    state.lapPoints.resize(100);
    state.race.lap = 2;
    state.race.segment = 10;
    state.opponents.resize(2);
    state.opponents[0].race.lap = 2;
    state.opponents[0].race.segment = 50;   // ahead on the same lap
    state.opponents[1].race.lap = 1;
    state.opponents[1].race.segment = 90;   // a lap down
    RankRacerField(state);
    EXPECT_EQ(state.race.position, 2);
    EXPECT_EQ(state.race.entrants, 3);
    EXPECT_EQ(state.opponents[0].race.position, 1);
    EXPECT_EQ(state.opponents[1].race.position, 3);
}

TEST(RacerField, FinisherOutranksRacers) {
    RacerRaceState racing;
    racing.lap = 3;
    racing.segment = 99;
    RacerRaceState finished;
    finished.finished = true;
    finished.raceTime = 200.f;
    EXPECT_GT(RacerRaceProgress(finished, 100),
              RacerRaceProgress(racing, 100));
}

TEST(RacerField, OverlappingPodsArePushedApart) {
    RacerPodState a;
    RacerPodState b;
    b.position = {1.f, 0.f, 0.f};
    for (int frame = 0; frame < 30; ++frame) SeparateRacerPods({&a, &b});
    EXPECT_GE(b.position.x - a.position.x, 7.9f);
}

TEST(RacerField, AiSteersRoundAPodAheadAndLiftsOff) {
    RacerPodState me;
    me.speed = 100.f;
    RacerPodState slow;
    slow.position = {2.f, 0.f, -12.f};  // ahead (heading 0 is -z), right
    slow.speed = 40.f;
    RacerPodInput input;
    input.throttle = 1.f;
    input.boost = true;
    AvoidRacerTraffic(me, {&slow}, input);
    EXPECT_LT(input.steer, 0.f);  // away to the left
    EXPECT_LE(input.throttle, 0.4f);
    EXPECT_FALSE(input.boost);
}

TEST(RacerField, FinishersSeparatedByHundredths) {
    RacerRaceState a;
    a.finished = true;
    a.raceTime = 67.98f;
    RacerRaceState b = a;
    b.raceTime = 68.04f;
    EXPECT_GT(RacerRaceProgress(a, 1080), RacerRaceProgress(b, 1080));
}

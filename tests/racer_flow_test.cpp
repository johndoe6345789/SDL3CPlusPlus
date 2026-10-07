#include "racer_flow_fixture.hpp"

#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"
#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;
using racer_test::RacerFlowTest;

TEST_F(RacerFlowTest, TitleMenuWrapsAndOpensScreens) {
    UpdateRacerMenu(flow_, Press(&RacerNav::up), table_);
    EXPECT_EQ(flow_.menuRow, 5);  // wrapped to QUIT
    flow_.menuRow = 0;
    UpdateRacerMenu(flow_, Press(&RacerNav::select), table_);
    EXPECT_EQ(flow_.phase, RacerPhase::Tournament);
}

TEST_F(RacerFlowTest, TournamentStartLoadsTheFirstTrack) {
    flow_.phase = RacerPhase::Tournament;
    flow_.setupRow = 4;  // START RACE
    UpdateRacerTournament(flow_, Press(&RacerNav::select), table_);
    EXPECT_EQ(flow_.phase, RacerPhase::Loading);
    EXPECT_TRUE(flow_.tournament);
    EXPECT_EQ(flow_.trackIndex, 0);
}

TEST_F(RacerFlowTest, ShopSellsStockedPartsAndTradesIn) {
    flow_.phase = RacerPhase::Shop;
    flow_.shopLevel = 5;
    UpdateRacerShop(flow_, Press(&RacerNav::right));
    EXPECT_EQ(flow_.shopLevel, 1);  // Watto has only R-60 at the start
    UpdateRacerShop(flow_, Press(&RacerNav::select));
    EXPECT_EQ(flow_.profile.upgrades[0], 1);
    EXPECT_EQ(flow_.profile.truguts, 400 - 400 + 62);  // R-20 trade-in
    EXPECT_EQ(LoadRacerProfile(path_).upgrades[0], 1);
}

TEST_F(RacerFlowTest, PitDroidsJoinUpToFour) {
    flow_.profile.truguts = 10000;
    for (int i = 0; i < 5; ++i) {
        UpdateRacerPitDroids(flow_, Press(&RacerNav::select));
    }
    EXPECT_EQ(flow_.profile.pitDroids, kRacerMaxPitDroids);
    EXPECT_EQ(flow_.profile.truguts, 10000 - 3 * kRacerPitDroidPrice);
}

TEST_F(RacerFlowTest, WornPartsGiveLess) {
    RacerProfile profile;
    profile.upgrades[3] = 4;  // TOP SPEED: Block5 Thrust Coil
    const RacerPodSpec stock;
    EXPECT_FLOAT_EQ(ApplyRacerUpgrades(stock, profile).topSpeed,
                    stock.topSpeed * 1.2f);
    profile.health[3] = 0.5f;
    EXPECT_FLOAT_EQ(ApplyRacerUpgrades(stock, profile).topSpeed,
                    stock.topSpeed * 1.1f);
}

TEST_F(RacerFlowTest, PauseAndResume) {
    RacerWorldState state;
    flow_.phase = RacerPhase::Racing;
    UpdateRacerRaceFlow(flow_, state, Press(&RacerNav::back), 0.f);
    EXPECT_EQ(flow_.phase, RacerPhase::Paused);
    UpdateRacerRaceFlow(flow_, state, Press(&RacerNav::select), 0.f);
    EXPECT_EQ(flow_.phase, RacerPhase::Racing);
}

TEST_F(RacerFlowTest, ATournamentRaceIsBookedOnce) {
    RacerWorldState state;
    state.race.finished = true;
    state.race.position = 1;
    flow_.tournament = true;
    flow_.phase = RacerPhase::Racing;
    UpdateRacerRaceFlow(flow_, state, RacerNav{}, 4.f);
    EXPECT_EQ(flow_.phase, RacerPhase::Results);
    const int purse = RacerPurseFor(0, RacerPurseSplit::Fair, 1);
    EXPECT_EQ(flow_.profile.truguts, 400 + purse);
    UpdateRacerRaceFlow(flow_, state, RacerNav{}, 4.f);
    EXPECT_EQ(flow_.profile.truguts, 400 + purse);
}

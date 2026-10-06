#include "racer_flow_fixture.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;
using racer_test::RacerFlowTest;


TEST_F(RacerFlowTest, MenuWrapsAndChangesTheTrack) {
    UpdateRacerMenu(flow_, Press(&RacerNav::up), table_);
    EXPECT_EQ(flow_.menuRow, 6);  // wrapped to QUIT
    flow_.menuRow = 0;
    UpdateRacerMenu(flow_, Press(&RacerNav::left), table_);
    EXPECT_EQ(flow_.trackIndex, 0);
    UpdateRacerMenu(flow_, Press(&RacerNav::left), table_);
    EXPECT_EQ(flow_.trackIndex, 24);  // wrapped
}

TEST_F(RacerFlowTest, StartRaceRequestsALoad) {
    flow_.menuRow = 5;
    UpdateRacerMenu(flow_, Press(&RacerNav::select), table_);
    EXPECT_EQ(flow_.phase, RacerPhase::Loading);
    EXPECT_TRUE(flow_.requestLoad);
}

TEST_F(RacerFlowTest, ShopSpendsTrugutsAndSaves) {
    flow_.phase = RacerPhase::Shop;
    flow_.profile.truguts = 300;
    UpdateRacerShop(flow_, Press(&RacerNav::select));  // TRACTION, 250
    EXPECT_EQ(flow_.profile.upgrades[0], 1);
    EXPECT_EQ(flow_.profile.truguts, 50);
    UpdateRacerShop(flow_, Press(&RacerNav::select));  // 500: too dear
    EXPECT_EQ(flow_.profile.upgrades[0], 1);
    EXPECT_EQ(LoadRacerProfile(path_).truguts, 50);
}

TEST_F(RacerFlowTest, UpgradesImproveThePod) {
    RacerProfile profile;
    profile.upgrades[3] = 4;  // TOP SPEED
    const RacerPodSpec stock;
    EXPECT_FLOAT_EQ(ApplyRacerUpgrades(stock, profile).topSpeed,
                    stock.topSpeed * 1.2f);
}

TEST_F(RacerFlowTest, PauseAndResume) {
    RacerWorldState state;
    flow_.phase = RacerPhase::Racing;
    UpdateRacerRaceFlow(flow_, state, Press(&RacerNav::back), 0.f);
    EXPECT_EQ(flow_.phase, RacerPhase::Paused);
    UpdateRacerRaceFlow(flow_, state, Press(&RacerNav::select), 0.f);
    EXPECT_EQ(flow_.phase, RacerPhase::Racing);
}

TEST_F(RacerFlowTest, FinishingAwardsThePrizeOnce) {
    RacerWorldState state;
    state.race.finished = true;
    state.race.position = 1;
    flow_.phase = RacerPhase::Racing;
    UpdateRacerRaceFlow(flow_, state, RacerNav{}, 4.f);
    EXPECT_EQ(flow_.phase, RacerPhase::Results);
    EXPECT_EQ(flow_.profile.truguts, RacerPrizeFor(1));
    UpdateRacerRaceFlow(flow_, state, RacerNav{}, 4.f);
    EXPECT_EQ(flow_.profile.truguts, RacerPrizeFor(1));
}

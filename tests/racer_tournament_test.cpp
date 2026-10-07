#include "services/interfaces/workflow/racer/flow/racer_parts.hpp"
#include "services/interfaces/workflow/racer/flow/racer_tournament.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace {

constexpr int kRacers = 25;

}  // namespace

TEST(RacerTournament, APodiumOpensTheNextTrack) {
    RacerProfile profile;
    const auto out = RecordRacerTournamentRace(
        profile, 0, 0, 2, RacerPurseSplit::Fair, kRacers);
    EXPECT_EQ(profile.tracksOpen[0], 2);
    EXPECT_EQ(profile.best[0], 2);
    EXPECT_EQ(profile.podiums, 1);
    EXPECT_EQ(out.prize, RacerPurseFor(0, RacerPurseSplit::Fair, 2));
    EXPECT_EQ(out.points, 8);
    EXPECT_EQ(out.newRacer, -1);  // only a win opens a racer
}

TEST(RacerTournament, AFirstWinOpensARacerOnce) {
    RacerProfile profile;
    auto out = RecordRacerTournamentRace(profile, 0, 0, 1,
                                         RacerPurseSplit::Fair, kRacers);
    EXPECT_EQ(out.newRacer, 7);
    EXPECT_TRUE(profile.racers & (1u << 7));
    out = RecordRacerTournamentRace(profile, 0, 0, 1,
                                    RacerPurseSplit::Fair, kRacers);
    EXPECT_EQ(out.newRacer, -1);
}

TEST(RacerTournament, ConqueringACircuitOpensTheNext) {
    RacerProfile profile;
    profile.tracksOpen[0] = 7;
    for (int slot = 0; slot < 6; ++slot) profile.best[slot] = 1;
    RecordRacerTournamentRace(profile, 0, 6, 3, RacerPurseSplit::Fair,
                              kRacers);
    EXPECT_EQ(profile.circuitsOpen, 2);
}

TEST(RacerTournament, WinnerTakesAllPaysOnlyTheWinner) {
    EXPECT_GT(RacerPurseFor(0, RacerPurseSplit::WinnerTakesAll, 1),
              RacerPurseFor(0, RacerPurseSplit::Fair, 1));
    EXPECT_EQ(RacerPurseFor(0, RacerPurseSplit::WinnerTakesAll, 2), 0);
    EXPECT_EQ(RacerPurseFor(0, RacerPurseSplit::Skilled, 4), 0);
    EXPECT_GT(RacerPurseFor(0, RacerPurseSplit::Fair, 4), 0);
}

TEST(RacerParts, RacesWearPartsAndDroidsMendThem) {
    RacerProfile profile;
    WearAndRepairRacerParts(profile, 0.5f, false);
    EXPECT_NEAR(profile.health[0], 1.f - 0.13f + 0.06f, 1e-4f);
    profile.pitDroids = 4;
    WearAndRepairRacerParts(profile, 0.f, false);
    EXPECT_FLOAT_EQ(profile.health[0], 1.f);
}

TEST(RacerParts, TheJunkyardSellsWornCheapParts) {
    RacerProfile profile;
    const auto stock = RacerJunkyardStock(profile);
    ASSERT_FALSE(stock.empty());
    for (const RacerJunkOffer& offer : stock) {
        EXPECT_LT(offer.health, 1.f);
        EXPECT_LT(offer.price, RacerPart(offer.type, offer.level).price);
        EXPECT_GE(offer.level, 1);
    }
}

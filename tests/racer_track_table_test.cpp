#include "services/interfaces/workflow/racer/data/racer_track_table.hpp"

#include <gtest/gtest.h>

using namespace sdl3cpp::services::impl;

namespace {

constexpr const char* kTable = "packages/racer/assets/racer_tracks.json";

}  // namespace

TEST(RacerTrackTable, ReadsTracksRacersAndPlanetFog) {
    const RacerTrackTable table = LoadRacerTrackTable(kTable);
    ASSERT_TRUE(table.loaded);
    EXPECT_EQ(table.tracks.size(), 25u);
    EXPECT_EQ(table.racers.size(), 25u);
    // Every planet has its own fog colour (this once read as none).
    EXPECT_EQ(table.fog.size(), 8u);
    ASSERT_TRUE(table.fog.count("Oovo IV"));
    EXPECT_FLOAT_EQ(table.fog.at("Oovo IV")[2], 0.16f);
}

TEST(RacerTrackTable, RacersCarryTheGamesHandling) {
    const RacerTrackTable table = LoadRacerTrackTable(kTable);
    const RacerPodInfo* sebulba = FindRacerPod(table, "Sebulba");
    ASSERT_NE(sebulba, nullptr);
    EXPECT_FLOAT_EQ(sebulba->handling[4], 600.f);  // max speed
}

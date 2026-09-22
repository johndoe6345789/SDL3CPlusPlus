#include "services/interfaces/workflow/stunts/world/stunts_track_mesh.hpp"

#include <gtest/gtest.h>

using sdl3cpp::services::impl::BuildStuntsTrackMesh;
using sdl3cpp::services::impl::kStuntsCells;
using sdl3cpp::services::impl::kStuntsGrid;
using sdl3cpp::services::impl::kStuntsLinkEast;
using sdl3cpp::services::impl::kStuntsLinkNorth;
using sdl3cpp::services::impl::kStuntsLinkSouth;
using sdl3cpp::services::impl::kStuntsLinkWest;
using sdl3cpp::services::impl::StuntsCellCentre;
using sdl3cpp::services::impl::StuntsMeshParams;
using sdl3cpp::services::impl::StuntsTileKind;
using sdl3cpp::services::impl::StuntsTileTable;
using sdl3cpp::services::impl::StuntsTrack;

namespace {

constexpr std::uint8_t kStraightNs = 4;
constexpr std::uint8_t kStraightEw = 5;
constexpr std::uint8_t kCornerEs = 0xfe;

StuntsTileTable MakeTable() {
    StuntsTileTable table;
    table.tiles[kStraightNs] = {StuntsTileKind::Straight,
                                kStuntsLinkNorth | kStuntsLinkSouth};
    table.tiles[kStraightEw] = {StuntsTileKind::Straight,
                                kStuntsLinkEast | kStuntsLinkWest};
    table.tiles[kCornerEs] = {StuntsTileKind::Corner,
                              kStuntsLinkEast | kStuntsLinkSouth};
    table.known = 3;
    table.loaded = true;
    return table;
}

StuntsTrack MakeTrack() {
    StuntsTrack track;
    track.loaded = true;
    track.road[0] = kCornerEs;
    track.road[1] = kStraightEw;
    track.road[kStuntsGrid] = kStraightNs;
    return track;
}

}  // namespace

TEST(StuntsTrackMesh, GroundCoversEveryCell) {
    const auto mesh =
        BuildStuntsTrackMesh(MakeTrack(), MakeTable(), StuntsMeshParams{});
    // Two triangles per cell, and nothing outside the fixed grid.
    EXPECT_EQ(mesh.ground.indices.size(),
              static_cast<std::size_t>(kStuntsCells) * 6u);
}

TEST(StuntsTrackMesh, CountsOnlyKnownRoadTiles) {
    const auto mesh =
        BuildStuntsTrackMesh(MakeTrack(), MakeTable(), StuntsMeshParams{});
    EXPECT_EQ(mesh.roadTiles, 3);
    EXPECT_FALSE(mesh.road.indices.empty());
}

TEST(StuntsTrackMesh, EmptyTrackMeshesNothing) {
    const auto mesh =
        BuildStuntsTrackMesh(StuntsTrack{}, MakeTable(), StuntsMeshParams{});
    EXPECT_EQ(mesh.roadTiles, 0);
    EXPECT_TRUE(mesh.ground.indices.empty());
}

TEST(StuntsTrackMesh, GridIsCentredOnTheOrigin) {
    const float tile = 24.f;
    const auto first = StuntsCellCentre(0, 0, tile);
    const auto last = StuntsCellCentre(kStuntsGrid - 1, kStuntsGrid - 1, tile);
    EXPECT_FLOAT_EQ(first.x, -last.x);
    EXPECT_FLOAT_EQ(first.z, -last.z);
    EXPECT_LT(first.z, last.z);  // row 0 is the north edge
}

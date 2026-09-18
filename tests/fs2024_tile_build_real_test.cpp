// One real level-14 tile built end to end from the installed game, the
// way the streaming loader builds every tile: FS2024's own elevation,
// ground cover and buildings for central Westminster. Skipped when the
// game is absent.

#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_building_mesh.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_tile_ground.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_library.hpp"
#include "services/interfaces/workflow/fs2024/world/fs2024_geo_origin.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>

namespace impl = sdl3cpp::services::impl;

namespace {

constexpr const char* kCglRoot = "D:/Games/Official/Steam/fs-base-cgl";

/// Central Westminster's own level-14 tile.
sdl3cpp::fs2024::QuadTile Westminster() {
    return sdl3cpp::fs2024::TileAtLatLon(51.5007, -0.1246, 14);
}

}  // namespace

TEST(Fs2024TileBuildReal, WestminsterGroundIsCentralLondon) {
    if (!std::filesystem::exists(kCglRoot)) GTEST_SKIP() << "no FS2024";
    const auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    impl::Fs2024DemSampler dem(kCglRoot);
    const impl::Fs2024Heightfield field = impl::BuildFs2024TileHeights(
        dem, Westminster().x, Westminster().y, 14, origin.TileSize(), 64);
    EXPECT_EQ(field.columns, 65);
    EXPECT_NEAR(field.spacing * 64.f, origin.TileSize(), 0.01f);
    EXPECT_GT(field.minHeight, -10.f);  // the Thames, not the sea bed
    EXPECT_LT(field.maxHeight, 60.f);   // no hills in Westminster

    impl::Fs2024ClassSampler classes(kCglRoot);
    const auto cover = impl::BuildFs2024TileClasses(
        classes, Westminster().x, Westminster().y, 14, 64);
    const auto urban = std::count(cover.begin(), cover.end(), 8);
    EXPECT_GT(urban, static_cast<long>(cover.size() * 3 / 4));
}

TEST(Fs2024TileBuildReal, WestminsterBuildingsStandOnTheGround) {
    if (!std::filesystem::exists(kCglRoot)) GTEST_SKIP() << "no FS2024";
    const auto origin = impl::MakeFs2024GeoOrigin(51.5007, -0.1246);
    impl::Fs2024DemSampler dem(kCglRoot);
    const impl::Fs2024Heightfield field = impl::BuildFs2024TileHeights(
        dem, Westminster().x, Westminster().y, 14, origin.TileSize(), 64);

    sdl3cpp::fs2024::BldLibrary library(kCglRoot);
    const auto tiles = library.ReadTile(Westminster());
    ASSERT_EQ(tiles.size(), 2u);  // surveyed and imagery-derived
    const auto plans = impl::PlanFs2024TileBuildings(tiles, origin.TileSize());
    EXPECT_GT(plans.size(), 500u);

    const auto mesh = impl::MeshFs2024TileBuildings(plans, field);
    ASSERT_FALSE(mesh.wallIndices.empty());
    ASSERT_FALSE(mesh.roofIndices.empty());
    for (const auto& vertex : mesh.wallVertices) {
        ASSERT_GE(vertex.y, field.minHeight - 0.01f);
    }
}

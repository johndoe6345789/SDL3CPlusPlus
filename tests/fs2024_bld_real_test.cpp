// The whole chain against FS2024's own installed data: the CGL
// container, the level-14 quadkey index and the BLD tile decoder,
// on the tile that covers Westminster. Skipped when the game is not
// installed on this machine -- there is nothing to substitute for
// 130 MB of the game's own London building data.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_tile.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <cmath>
#include <string>

namespace tools = sdl3cpp::fs2024;

namespace {

constexpr const char* kLondonBuildings =
    "D:/Games/Official/Steam/fs-base-cgl/CGL/031/bldn313.cgl";

/// The tile's key inside its container: a level-14 quadkey minus the
/// six digits the folder and file name already spell out.
std::uint32_t KeyBelowContainer(const std::string& quadKey) {
    std::uint32_t key = 0;
    for (std::size_t i = 6; i < quadKey.size(); ++i) {
        key = key * 4 + static_cast<std::uint32_t>(quadKey[i] - '0');
    }
    return key;
}

}  // namespace

TEST(Fs2024BldReal, WestminsterTileDecodesToItsRealBuildings) {
    if (!std::filesystem::exists(kLondonBuildings)) {
        GTEST_SKIP() << "FS2024 not installed";
    }
    const auto container = tools::ReadCglContainer(kLondonBuildings);
    EXPECT_EQ(container.tiles.size(), 30553u);

    const tools::QuadTile quad = tools::TileAtLatLon(51.5007, -0.1246, 14);
    const std::string quadKey = tools::QuadKeyOf(quad);
    EXPECT_EQ(quadKey, "03131313113010");

    const tools::CglTileEntry* entry =
        tools::FindCglTile(container, KeyBelowContainer(quadKey));
    ASSERT_NE(entry, nullptr);
    const auto blob = tools::ReadCglTile(container, *entry);
    ASSERT_EQ(blob.size(), 21180u);

    const tools::BldTile tile = tools::DecodeBldTile(blob, 6);
    EXPECT_EQ(tile.bytesRead, blob.size());
    ASSERT_EQ(tile.buildings.size(), 542u);

    const tools::BldBuilding& first = tile.buildings.front();
    EXPECT_EQ(first.flags, 114);
    ASSERT_EQ(first.ringSizes.size(), 1u);
    EXPECT_EQ(first.ringSizes[0], 4u);
    EXPECT_EQ(first.roofType, 1);
    EXPECT_TRUE(first.hasRoofColour);
    EXPECT_EQ(first.red, 20);
    EXPECT_EQ(first.green, 21);
    EXPECT_EQ(first.blue, 20);
    ASSERT_EQ(first.vertices.size(), 4u);
    EXPECT_EQ(first.vertices[0].x, -1707);
    EXPECT_EQ(first.vertices[0].y, 8239);

}

namespace {

double Area(const tools::BldBuilding& b) {
    double twice = 0.0;
    const std::uint32_t n = b.ringSizes[0];
    for (std::uint32_t i = 0; i < n; ++i) {
        const auto& p = b.vertices[i];
        const auto& q = b.vertices[(i + 1) % n];
        twice += static_cast<double>(p.x) * q.y - static_cast<double>(q.x) * p.y;
    }
    return std::abs(twice) / 2.0;
}

}  // namespace

TEST(Fs2024BldReal, CountyHallAndTheTreasuryStandWhereTheyReallyDo) {
    // Ground truth that owes nothing to any decoder: the tile's two
    // largest six-to-seven storey footprints are County Hall and HM
    // Treasury, and must land on their real centres. The old reading
    // (north-west origin, +y south) put them half a tile away and
    // mirrored north-south.
    if (!std::filesystem::exists(kLondonBuildings)) {
        GTEST_SKIP() << "FS2024 not installed";
    }
    const auto container = tools::ReadCglContainer(
        "D:/Games/Official/Steam/fs-base-cgl/CGL/031/bldo313.cgl");
    const tools::QuadTile quad = tools::TileAtLatLon(51.5007, -0.1246, 14);
    const auto blob = tools::ReadCglTile(
        container, *tools::FindCglTile(container, KeyBelowContainer(
                                                      tools::QuadKeyOf(quad))));
    const tools::BldTile tile = tools::DecodeBldTile(blob, 6);
    struct Truth { const char* name; double lat, lon; };
    for (const Truth& truth : {Truth{"County Hall", 51.5018, -0.1195},
                               Truth{"HM Treasury", 51.5018, -0.1283}}) {
        double best = 1e9;
        const tools::BldBuilding* found = nullptr;
        for (const auto& b : tile.buildings) {
            if (b.ringSizes.empty() || Area(b) < 2.0e6) continue;  // ~17k m2
            double cx = 0, cy = 0;
            for (std::uint32_t i = 0; i < b.ringSizes[0]; ++i) {
                cx += b.vertices[i].x; cy += b.vertices[i].y;
            }
            double lon = 0, lat = 0;
            tools::BldVertexToLatLon(
                quad, static_cast<std::int32_t>(cx / b.ringSizes[0]),
                static_cast<std::int32_t>(cy / b.ringSizes[0]), lon, lat);
            const double metres = std::hypot((lon - truth.lon) * 69300.0,
                                             (lat - truth.lat) * 111320.0);
            if (metres < best) { best = metres; found = &b; }
        }
        ASSERT_NE(found, nullptr) << truth.name;
        EXPECT_LT(best, 60.0) << truth.name;
    }
}

TEST(Fs2024BldReal, MostOfATilesBuildingsAreInsideIt) {
    // Centred coordinates: about 70% of a tile's records lie within
    // +/-8192 of its centre, the rest straddle its edges.
    if (!std::filesystem::exists(kLondonBuildings)) {
        GTEST_SKIP() << "FS2024 not installed";
    }
    const auto container = tools::ReadCglContainer(
        "D:/Games/Official/Steam/fs-base-cgl/CGL/031/bldo313.cgl");
    const tools::QuadTile quad = tools::TileAtLatLon(51.5007, -0.1246, 14);
    const auto blob = tools::ReadCglTile(
        container, *tools::FindCglTile(container, KeyBelowContainer(
                                                      tools::QuadKeyOf(quad))));
    const tools::BldTile tile = tools::DecodeBldTile(blob, 6);
    int inside = 0;
    for (const auto& b : tile.buildings) {
        const auto& v = b.vertices.front();
        if (std::abs(v.x) < 8192 && std::abs(v.y) < 8192) ++inside;
    }
    EXPECT_GT(inside, static_cast<int>(tile.buildings.size() * 6 / 10));
}

TEST(Fs2024BldReal, EveryLondonTileParsesToItsExactLength) {
    if (!std::filesystem::exists(kLondonBuildings)) {
        GTEST_SKIP() << "FS2024 not installed";
    }
    const auto container = tools::ReadCglContainer(kLondonBuildings);
    std::size_t checked = 0, buildings = 0;
    for (std::size_t i = 0; i < container.tiles.size(); i += 250) {
        const auto blob = tools::ReadCglTile(container, container.tiles[i]);
        const tools::BldTile tile = tools::DecodeBldTile(blob, 6);
        EXPECT_EQ(tile.bytesRead, blob.size()) << "tile " << i;
        buildings += tile.buildings.size();
        ++checked;
    }
    EXPECT_GT(checked, 100u);
    EXPECT_GT(buildings, 10000u);
}

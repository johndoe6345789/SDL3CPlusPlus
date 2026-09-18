// Turning FS2024's vector features into ground geometry: water rings
// clipped to their tile, laid flat with the ground carved below, roads
// draped as ribbons, bridges held clear of the water, and open water
// found where the DEM holds a still surface.

#include "services/interfaces/workflow/fs2024/assemble/fs2024_road_build.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_sea_build.hpp"

#include <gtest/gtest.h>

#include <cmath>

namespace s = sdl3cpp::services::impl;

namespace {

/// A 100 m field, 11 x 11, at `height`, plus `bump` metres rising east.
s::Fs2024Heightfield Field(float height, float bump = 0.f) {
    s::Fs2024Heightfield field;
    field.columns = field.rows = 11;
    field.spacing = 10.f;
    for (int r = 0; r < 11; ++r) {
        for (int c = 0; c < 11; ++c) field.heights.push_back(height + bump * c);
    }
    field.minHeight = height;
    field.maxHeight = height + bump * 10.f;
    return field;
}

s::Fs2024VecShape Ring(std::vector<s::Point2> points) {
    s::Fs2024VecShape shape;
    shape.points = std::move(points);
    return shape;
}

double Area(const std::vector<s::Point2>& ring) {
    double area = 0.0;
    for (std::size_t i = 0; i < ring.size(); ++i) {
        const auto& a = ring[i];
        const auto& b = ring[(i + 1) % ring.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return std::abs(area) / 2.0;
}

}  // namespace

TEST(Fs2024VecBuild, ClipsARingToItsTile) {
    // A 60 x 60 square hanging 20 m off the tile's west and north edges.
    const auto clipped =
        s::ClipRingToSquare({{-20, -20}, {40, -20}, {40, 40}, {-20, 40}}, 100);
    EXPECT_NEAR(Area(clipped), 40.0 * 40.0, 1e-3);
    for (const auto& p : clipped) {
        EXPECT_GE(p.x, 0.f);
        EXPECT_GE(p.y, 0.f);
    }
}

TEST(Fs2024VecBuild, WaterLiesFlatWithTheGroundCarvedBelow) {
    auto field = Field(10.f, 0.5f);  // 10 m in the west to 15 m east
    const std::vector<s::Fs2024VecShape> water = {
        Ring({{25, 25}, {75, 25}, {75, 75}, {25, 75}})};
    const auto build = s::BuildFs2024Water(water, field);
    ASSERT_EQ(build.levels.size(), 1u);
    EXPECT_NEAR(build.levels[0], 10.f + 0.5f * 2.5f, 1e-3);  // x = 25 m
    EXPECT_EQ(build.mesh.indices.size(), 6u);  // one quad
    for (const auto& v : build.mesh.vertices) {
        EXPECT_FLOAT_EQ(v.y, build.levels[0]);
    }
    EXPECT_LE(field.At(5, 5), build.levels[0] - 3.f);  // inside: carved
    EXPECT_FLOAT_EQ(field.At(0, 0), 10.f);             // outside: as was
}

TEST(Fs2024VecBuild, RoadsDrapeAtTheirWidthAndBridgesClearTheWater) {
    auto field = Field(5.f);
    s::Fs2024VecShape road;
    road.classBit = 21;
    road.points = {{0, 50}, {100, 50}};
    const auto mesh = s::BuildFs2024Roads({road}, {}, {}, field);
    ASSERT_FALSE(mesh.vertices.empty());
    const float width = s::Fs2024RoadWidth(21);
    EXPECT_NEAR(mesh.max.z - mesh.min.z, width, 1e-3);
    EXPECT_NEAR(mesh.max.y, 5.3f, 1e-3);

    const std::vector<s::Fs2024VecShape> water = {
        Ring({{30, 0}, {70, 0}, {70, 100}, {30, 100}})};
    road.flags = 0x80;  // a bridge
    road.level = 1;
    const auto bridge = s::BuildFs2024Roads({road}, water, {2.f}, field);
    EXPECT_NEAR(bridge.max.y, 2.f + 5.f + 0.3f, 1e-3);  // over the river
    s::Fs2024VecShape footway = road;
    footway.classBit = 30;
    EXPECT_TRUE(s::BuildFs2024Roads({footway}, {}, {}, field).indices.empty());
}

TEST(Fs2024VecBuild, OpenWaterIsWhereTheDemHoldsStill) {
    auto sea = Field(-2.33f);
    s::Fs2024TerrainChunkMesh water;
    water.min = glm::vec3(1e30f);
    water.max = glm::vec3(-1e30f);
    s::AddFs2024OpenWater({}, sea, water);
    EXPECT_EQ(water.indices.size(), 100u * 6u);  // every cell
    EXPECT_FLOAT_EQ(water.max.y, -2.33f);
    EXPECT_LE(sea.maxHeight, 0.f);

    auto land = Field(3.f, 0.1f);  // a gentle slope: land
    s::Fs2024TerrainChunkMesh none;
    s::AddFs2024OpenWater({}, land, none);
    EXPECT_TRUE(none.indices.empty());

    // Where the vector layer maps the ground, its own outlines rule.
    s::Fs2024TileShapes mapped;
    mapped.side = 1;
    mapped.quadSize = 100.f;
    mapped.mapped = {true};
    auto coast = Field(-2.33f);
    s::Fs2024TerrainChunkMesh skipped;
    s::AddFs2024OpenWater(mapped, coast, skipped);
    EXPECT_TRUE(skipped.indices.empty());
}

// The fs2024 prepare tool's pure-logic pieces: argument parsing (a
// real bug was found here -- an inverted XOR check accepted neither
// mode and rejected either valid one), grid alignment, and polygon
// triangulation/extrusion.

#include "services/interfaces/workflow/fs2024/prepare/fs2024_grid_layout.hpp"
#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_catalog.hpp"
#include "services/interfaces/workflow/fs2024/prepare/landmark/fs2024_landmark_placement.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_local_frame.hpp"
#include "services/interfaces/workflow/fs2024/prepare/fs2024_prepare_args.hpp"
#include "services/interfaces/workflow/fs2024/building/fs2024_building_mesh.hpp"
#include "services/interfaces/workflow/fs2024/building/fs2024_polygon.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace tools = sdl3cpp::tools::fs2024;
namespace impl = sdl3cpp::services::impl;

namespace {

/// argv-style helper: `args` are string literals, argv[0] is a dummy
/// program name as real argv always has.
tools::PrepareArgs Parse(std::vector<const char*> args) {
    std::vector<char*> argv{const_cast<char*>("fs2024_prepare")};
    for (const char* a : args) argv.push_back(const_cast<char*>(a));
    return tools::ParsePrepareArgs(static_cast<int>(argv.size()),
                                   argv.data());
}

}  // namespace

TEST(Fs2024PrepareArgs, AcceptsAirportModeAlone) {
    const auto args = Parse({"--icao", "LOWI", "--dem", "d.tif", "--out",
                             "o"});
    EXPECT_EQ(args.icao, "LOWI");
    EXPECT_FALSE(args.hasLatLon);
}

TEST(Fs2024PrepareArgs, AcceptsRoadModeAlone) {
    const auto args =
        Parse({"--lat", "51.5", "--lon", "-0.1", "--osm-json", "o.json",
              "--dem", "d.tif", "--out", "o"});
    EXPECT_TRUE(args.hasLatLon);
    EXPECT_DOUBLE_EQ(args.lat, 51.5);
}

TEST(Fs2024PrepareArgs, RejectsNeitherModeSelected) {
    EXPECT_THROW(Parse({"--dem", "d.tif", "--out", "o"}), std::runtime_error);
}

TEST(Fs2024PrepareArgs, RejectsBothModesSelected) {
    EXPECT_THROW(Parse({"--icao", "LOWI", "--lat", "51.5", "--lon", "-0.1",
                       "--osm-json", "o.json", "--dem", "d.tif", "--out",
                       "o"}),
                std::runtime_error);
}

TEST(Fs2024PrepareArgs, RoadModeWithoutOsmJsonIsRejected) {
    EXPECT_THROW(Parse({"--lat", "51.5", "--lon", "-0.1", "--dem", "d.tif",
                       "--out", "o"}),
                std::runtime_error);
}

TEST(Fs2024GridLayout, CorrectsATileSizeThatDoesNotDivideSpacing) {
    // 1000 / 16 = 62.5: rounds to 63 cells (C++'s round-half-away-from-
    // zero), a 1008 m true pitch -- not Python's 992, and that is fine;
    // what must hold is internal consistency, checked below.
    const auto grid = tools::ComputeGridLayout(4000.f, 1000.f, 16.f);
    EXPECT_NEAR(std::fmod(grid.originX / grid.tileSize + 1000.f, 1.f), 0.f,
               1e-4f);
    EXPECT_FLOAT_EQ((grid.cells - 1) * 16.f, grid.extent);
}

TEST(Fs2024GridLayout, NeverBakesFewerThanTheMinimumRadius) {
    const auto grid = tools::ComputeGridLayout(10.f, 1000.f, 16.f, 3);
    const int tilesPerSide =
        static_cast<int>(std::lround(grid.extent / grid.tileSize));
    EXPECT_EQ(tilesPerSide, 2 * 3 + 1);
}

TEST(Fs2024Polygon, TriangulatesASquareIntoTwoTriangles) {
    const std::vector<impl::Point2> square{
        {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}};
    const auto triangles = impl::TriangulatePolygon(square);
    EXPECT_EQ(triangles.size(), 6u);  // 2 triangles * 3 indices
}

TEST(Fs2024Polygon, DropsAClosedRingsDuplicateFirstLastPoint) {
    const std::vector<impl::Point2> ring{
        {0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}, {0.f, 1.f}, {0.f, 0.f}};
    EXPECT_EQ(impl::TriangulatePolygon(ring).size(), 6u);
}

TEST(Fs2024Polygon, TooFewPointsProducesNoTriangles) {
    EXPECT_TRUE(
        impl::TriangulatePolygon({{0.f, 0.f}, {1.f, 0.f}}).empty());
}

TEST(Fs2024BuildingMesh, ProducesWallsAndARoofForASquareFootprint) {
    const std::vector<impl::Point2> square{
        {0.f, 0.f}, {10.f, 0.f}, {10.f, 10.f}, {0.f, 10.f}};
    std::vector<impl::BspRenderVertex> wallVertices, roofVertices;
    std::vector<std::uint32_t> wallIndices, roofIndices;
    impl::AppendBuildingMesh(square, 9.f, wallVertices, wallIndices,
                            roofVertices, roofIndices);

    EXPECT_EQ(wallVertices.size(), 16u);  // 4 walls * 4 verts
    EXPECT_EQ(wallIndices.size(), 4u * 2u * 3u);  // 4 walls * 2 tris
    EXPECT_EQ(roofVertices.size(), 4u);
    EXPECT_EQ(roofIndices.size(), 2u * 3u);  // a square roof is 2 tris
    for (std::uint32_t index : wallIndices) {
        ASSERT_LT(index, wallVertices.size());
    }
    for (std::uint32_t index : roofIndices) {
        ASSERT_LT(index, roofVertices.size());
    }
}

TEST(Fs2024BuildingMesh, RoofSitsAtTheGivenHeightFacingUp) {
    const std::vector<impl::Point2> square{
        {0.f, 0.f}, {10.f, 0.f}, {10.f, 10.f}, {0.f, 10.f}};
    std::vector<impl::BspRenderVertex> wallVertices, roofVertices;
    std::vector<std::uint32_t> wallIndices, roofIndices;
    impl::AppendBuildingMesh(square, 9.f, wallVertices, wallIndices,
                            roofVertices, roofIndices);

    ASSERT_EQ(roofVertices.size(), 4u);
    for (const auto& v : roofVertices) {
        EXPECT_FLOAT_EQ(v.ny, 1.f);
        EXPECT_FLOAT_EQ(v.y, 9.f);
    }
}

TEST(Fs2024BuildingMesh, WallAndRoofUvsTileByWorldDistance) {
    // A wall 8 m long and 4 m tall, and a roof whose footprint corner
    // sits 8 m out -- both should read as exactly 2 texture tiles at
    // this engine's fixed 4 m/8 m wall/roof tile size.
    const std::vector<impl::Point2> square{
        {0.f, 0.f}, {8.f, 0.f}, {8.f, 8.f}, {0.f, 8.f}};
    std::vector<impl::BspRenderVertex> wallVertices, roofVertices;
    std::vector<std::uint32_t> wallIndices, roofIndices;
    impl::AppendBuildingMesh(square, 4.f, wallVertices, wallIndices,
                            roofVertices, roofIndices);

    ASSERT_EQ(wallVertices.size(), 16u);
    EXPECT_FLOAT_EQ(wallVertices[0].u, 0.f);
    EXPECT_FLOAT_EQ(wallVertices[1].u, 2.f);
    EXPECT_FLOAT_EQ(wallVertices[2].v, 1.f);

    // Winding normalisation may reorder/reverse the footprint, so check
    // the tiling scale by extent rather than assuming a vertex index.
    ASSERT_EQ(roofVertices.size(), 4u);
    float maxU = 0.f, maxV = 0.f;
    for (const auto& v : roofVertices) {
        maxU = std::max(maxU, v.u);
        maxV = std::max(maxV, v.v);
    }
    EXPECT_FLOAT_EQ(maxU, 1.f);
    EXPECT_FLOAT_EQ(maxV, 1.f);
}

TEST(Fs2024BuildingMesh, NoHeightOrTooFewPointsProducesNothing) {
    std::vector<impl::BspRenderVertex> wallVertices, roofVertices;
    std::vector<std::uint32_t> wallIndices, roofIndices;
    impl::AppendBuildingMesh({{0.f, 0.f}, {1.f, 0.f}, {1.f, 1.f}}, 0.f,
                            wallVertices, wallIndices, roofVertices,
                            roofIndices);
    EXPECT_TRUE(wallVertices.empty());
    EXPECT_TRUE(roofVertices.empty());
    impl::AppendBuildingMesh({{0.f, 0.f}, {1.f, 0.f}}, 9.f, wallVertices,
                            wallIndices, roofVertices, roofIndices);
    EXPECT_TRUE(wallVertices.empty());
    EXPECT_TRUE(roofVertices.empty());
}

namespace {

std::filesystem::path WriteTempFile(const std::string& name,
                                    const std::string& contents) {
    const auto path = std::filesystem::temp_directory_path() / name;
    std::ofstream(path) << contents;
    return path;
}

}  // namespace

TEST(Fs2024LandmarkCatalog, MissingPathIsEmptyNotAnError) {
    EXPECT_TRUE(tools::ReadLandmarkCatalog("").empty());
    EXPECT_TRUE(
        tools::ReadLandmarkCatalog("no_such_file_at_all.json").empty());
}

TEST(Fs2024LandmarkCatalog, ParsesAllFieldsOfEachEntry) {
    const auto path = WriteTempFile(
        "fs2024_landmark_catalog_test.json",
        R"({"landmarks": [{"match": "Elizabeth Tower", )"
        R"("bgl": "poi.bgl", "texturesDir": "tex", )"
        R"("model": "WestminsterPalace", "headingDegrees": 12.5}]})");
    const auto entries = tools::ReadLandmarkCatalog(path.string());
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].match, "Elizabeth Tower");
    EXPECT_EQ(entries[0].bglPath, "poi.bgl");
    EXPECT_EQ(entries[0].texturesDir, "tex");
    EXPECT_EQ(entries[0].model, "WestminsterPalace");
    EXPECT_FLOAT_EQ(entries[0].headingDegrees, 12.5f);
    std::filesystem::remove(path);
}

TEST(Fs2024LandmarkCatalog, EntryMissingMatchOrModelIsSkipped) {
    const auto path = WriteTempFile(
        "fs2024_landmark_catalog_test_skip.json",
        R"({"landmarks": [{"bgl": "poi.bgl", "model": "X"}, )"
        R"({"match": "Y", "bgl": "poi.bgl"}]})");
    EXPECT_TRUE(tools::ReadLandmarkCatalog(path.string()).empty());
    std::filesystem::remove(path);
}

TEST(Fs2024LandmarkPlacement, MatchesCaseInsensitiveSubstringAndCentres) {
    // A frame centred exactly on the square's own centroid: the match
    // should land back on (0, 0) in engine space.
    const tools::LocalFrame frame(-0.1246, 51.5007, 0.f);
    const tools::OsmWay tower{
        "Elizabeth Tower", "yes", 0.f,
        {{-0.1247, 51.5006}, {-0.1245, 51.5006}, {-0.1245, 51.5008},
        {-0.1247, 51.5008}}};
    const std::vector<tools::LandmarkCatalogEntry> catalog{
        {"elizabeth tower", "poi.bgl", "tex", "WestminsterPalace", 45.f}};

    const auto instances = tools::MatchLandmarks({tower}, catalog, frame);
    ASSERT_EQ(instances.size(), 1u);
    EXPECT_EQ(instances[0].model, "WestminsterPalace");
    EXPECT_FLOAT_EQ(instances[0].headingDegrees, 45.f);
    EXPECT_NEAR(instances[0].x, 0.f, 1.f);
    EXPECT_NEAR(instances[0].z, 0.f, 1.f);
}

TEST(Fs2024LandmarkPlacement, NoMatchingNameProducesNoInstance) {
    const tools::LocalFrame frame(0.0, 0.0, 0.f);
    const tools::OsmWay other{
        "Some Other Building", "yes", 0.f,
        {{0.0, 0.0}, {0.001, 0.0}, {0.001, 0.001}}};
    const std::vector<tools::LandmarkCatalogEntry> catalog{
        {"elizabeth tower", "poi.bgl", "tex", "WestminsterPalace", 0.f}};
    EXPECT_TRUE(tools::MatchLandmarks({other}, catalog, frame).empty());
}

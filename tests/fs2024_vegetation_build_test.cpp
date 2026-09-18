// Standing FS2024's own trees on its own forest, from a real installed
// species/biome library. Skipped when the game is absent.

#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_build.hpp"
#include "services/interfaces/workflow/fs2024/assemble/fs2024_vegetation_pick.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace f = sdl3cpp::fs2024;
namespace s = sdl3cpp::services::impl;

namespace {

constexpr const char* kVegetationRoot =
    "D:/Games/Official/Steam/fs-base/vegetation";
constexpr const char* kMaterialLibraryRoot =
    "D:/Games/Official/Steam/fs-base-vegetation-material-lib/MaterialLibs/"
    "Vegetation_MaterialLib";

/// A flat 200 m tile, half forest (west) and half grassland (east).
s::Fs2024Heightfield FlatField(float height) {
    s::Fs2024Heightfield field;
    field.columns = field.rows = 5;
    field.spacing = 50.f;
    field.heights.assign(25, height);
    field.minHeight = field.maxHeight = height;
    return field;
}

std::vector<std::uint8_t> HalfForest(int size) {
    std::vector<std::uint8_t> classes(static_cast<std::size_t>(size) * size);
    for (int r = 0; r < size; ++r) {
        for (int c = 0; c < size; ++c) {
            classes[static_cast<std::size_t>(r) * size + c] =
                c < size / 2 ? 2 : 3;  // forest west, grassland east
        }
    }
    return classes;
}

}  // namespace

TEST(Fs2024VegetationPick, PicksARealBiomeByLandClassAndLatitude) {
    EXPECT_EQ(s::Fs2024VegetationBiomeFor(2, 60.0), "Conifer Cold PNV fallback");
    EXPECT_EQ(s::Fs2024VegetationBiomeFor(2, 51.5), "Mixed Cold PNV fallback");
    EXPECT_EQ(s::Fs2024VegetationBiomeFor(2, 5.0), "Rain PNV fallback");
    EXPECT_EQ(s::Fs2024VegetationBiomeFor(4, 10.0), "Semi-Desert PNV fallback");
    EXPECT_EQ(s::Fs2024VegetationBiomeFor(3, 51.5), "");  // grassland: none
    EXPECT_EQ(s::Fs2024VegetationBiomeFor(9, 51.5), "");  // bare: none
}

TEST(Fs2024VegetationBuild, StandsTreesOnlyOnForestAndNotOnGrassland) {
    if (!std::filesystem::exists(kVegetationRoot)) GTEST_SKIP() << "no FS2024";
    const auto library =
        f::ReadVegetationLibrary(kVegetationRoot, kMaterialLibraryRoot);
    const auto field = FlatField(50.f);
    const auto classes = HalfForest(32);
    const auto groups =
        s::BuildFs2024Vegetation(library, classes, 32, field, 200.f, 51.5,
                                 12345ull);
    ASSERT_FALSE(groups.empty());
    std::size_t totalVerts = 0;
    for (const auto& group : groups) {
        EXPECT_FALSE(group.albedoPath.empty());
        EXPECT_TRUE(std::filesystem::exists(group.albedoPath));
        ASSERT_FALSE(group.mesh.vertices.empty());
        totalVerts += group.mesh.vertices.size();
        for (const auto& v : group.mesh.vertices) {
            EXPECT_LT(v.x, 100.f) << "a tree stood east, on grassland";
            // relativeOffsetY sinks a billboard below the ground on
            // purpose (it crops the atlas frame's own empty margin),
            // so only rule out something wildly wrong.
            EXPECT_GT(v.y, field.minHeight - 60.f);
            EXPECT_LT(v.y, field.maxHeight + 60.f);
        }
    }
    EXPECT_GT(totalVerts, 40u);  // several trees, not just one
}

TEST(Fs2024VegetationBuild, IsDeterministicAcrossRuns) {
    if (!std::filesystem::exists(kVegetationRoot)) GTEST_SKIP() << "no FS2024";
    const auto library =
        f::ReadVegetationLibrary(kVegetationRoot, kMaterialLibraryRoot);
    const auto field = FlatField(0.f);
    const auto classes = HalfForest(16);
    const auto a =
        s::BuildFs2024Vegetation(library, classes, 16, field, 200.f, 51.5, 7);
    const auto b =
        s::BuildFs2024Vegetation(library, classes, 16, field, 200.f, 51.5, 7);
    ASSERT_EQ(a.size(), b.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        ASSERT_EQ(a[i].mesh.vertices.size(), b[i].mesh.vertices.size());
        EXPECT_FLOAT_EQ(a[i].mesh.vertices[0].x, b[i].mesh.vertices[0].x);
    }
    const auto c =
        s::BuildFs2024Vegetation(library, classes, 16, field, 200.f, 51.5, 8);
    bool anyDifferent = a.size() != c.size();
    for (std::size_t i = 0; i < a.size() && !anyDifferent; ++i) {
        anyDifferent = c.size() <= i ||
                      a[i].mesh.vertices.size() != c[i].mesh.vertices.size();
    }
    EXPECT_TRUE(anyDifferent) << "a different tile seed should place trees "
                               "differently";
}

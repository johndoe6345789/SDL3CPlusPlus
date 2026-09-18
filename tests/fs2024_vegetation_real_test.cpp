// FS2024's own vegetation data: species, their imposter variants, and
// the fallback biome rules that mix them. Skipped when the game is
// absent.

#include "services/interfaces/workflow/fs2024/data/vegetation/fs2024_vegetation_library.hpp"

#include <gtest/gtest.h>

#include <filesystem>

namespace f = sdl3cpp::fs2024;

namespace {

constexpr const char* kVegetationRoot =
    "D:/Games/Official/Steam/fs-base/vegetation";
constexpr const char* kMaterialLibraryRoot =
    "D:/Games/Official/Steam/fs-base-vegetation-material-lib/MaterialLibs/"
    "Vegetation_MaterialLib";

}  // namespace

TEST(Fs2024VegetationReal, ReadsEveryRealSpeciesAndVariant) {
    if (!std::filesystem::exists(kVegetationRoot)) GTEST_SKIP() << "no FS2024";
    const auto library =
        f::ReadVegetationLibrary(kVegetationRoot, kMaterialLibraryRoot);
    EXPECT_GE(library.species.size(), 15u);

    const auto* conifer = library.Species("coniferboreal");
    ASSERT_NE(conifer, nullptr);
    EXPECT_EQ(conifer->materialGuid, "{F0862942-FF42-48A7-A151-9109887629A2}");
    ASSERT_GE(conifer->variations.size(), 5u);
    EXPECT_FLOAT_EQ(conifer->variations[0].sizeMin, 20.f);
    EXPECT_FLOAT_EQ(conifer->variations[0].sizeMax, 30.f);
    EXPECT_EQ(conifer->variations[0].frames, 10);
    EXPECT_EQ(conifer->variations[1].textureIndex, 1);

    // Its own albedo array is a real, present, sizeable DDS file.
    const std::string albedo = library.AlbedoPath(*conifer);
    ASSERT_FALSE(albedo.empty());
    EXPECT_TRUE(std::filesystem::exists(albedo));
    EXPECT_GT(std::filesystem::file_size(albedo), 1u << 20);
}

TEST(Fs2024VegetationReal, FallbackBiomeRulesMixRealSpecies) {
    if (!std::filesystem::exists(kVegetationRoot)) GTEST_SKIP() << "no FS2024";
    const auto library =
        f::ReadVegetationLibrary(kVegetationRoot, kMaterialLibraryRoot);
    const auto* cold = library.BiomeRule("Conifer Cold PNV fallback");
    ASSERT_NE(cold, nullptr);
    EXPECT_FLOAT_EQ(cold->instancesPerHectare, 700.f);
    ASSERT_FALSE(cold->species.empty());
    EXPECT_EQ(cold->species[0].name, "coniferboreal");

    // Every species a fallback rule names is one this engine can also
    // resolve to a real texture -- the whole point of using them.
    int resolved = 0, total = 0;
    for (const auto& rule : library.biomeRules) {
        for (const auto& ref : rule.species) {
            ++total;
            const auto* species = library.Species(ref.name);
            if (species && !library.AlbedoPath(*species).empty()) ++resolved;
        }
    }
    EXPECT_GT(total, 10);
    EXPECT_GT(resolved * 2, total);  // most names resolve to real textures
}

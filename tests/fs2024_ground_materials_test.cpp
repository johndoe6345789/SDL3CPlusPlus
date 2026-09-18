// Fs2024GroundMaterials against a hand-written slice of FS2024's own
// array index, then against the real one when the game is installed.

#include "services/interfaces/workflow/fs2024/data/material/fs2024_ground_materials.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace f = sdl3cpp::fs2024;

namespace {

/// The same shape as autogen/arrays.xml: climate-keyed grass, density-
/// keyed urban, and the meadow and support forest the fallbacks use.
std::string WriteFixture() {
    const auto path = std::filesystem::temp_directory_path() / "arrays.xml";
    std::ofstream(path) << R"(<root><array>
<biome land_classifications="2003" name="g" texture_file_prefix="t">
<id>2</id><type>0</type><start_index>10</start_index>
<variations>10</variations></biome>
<biome land_classifications="1000002008" name="u" texture_file_prefix="u">
<id>9</id><type>0</type><start_index>138</start_index>
<variations>3</variations></biome>
<biome land_classifications="2000002008" name="s" texture_file_prefix="s">
<id>10</id><type>0</type><start_index>144</start_index>
<variations>3</variations></biome>
<biome land_classifications="29" name="m" texture_file_prefix="m">
<id>40</id><type>0</type><start_index>272</start_index>
<variations>3</variations></biome>
<biome land_classifications="20" name="f" texture_file_prefix="f">
<id>30</id><type>0</type><start_index>236</start_index>
<variations>1</variations></biome>
</array></root>)";
    return path.string();
}

}  // namespace

TEST(Fs2024GroundMaterials, ClimateAndDensityPickTheirOwnLayers) {
    const auto materials = f::Fs2024GroundMaterials::Read(WriteFixture());
    EXPECT_EQ(materials.Count(), 5u);
    EXPECT_EQ(materials.For(3, 2).firstLayer, 10);
    EXPECT_EQ(materials.For(3, 2).layers, 10);
    EXPECT_EQ(materials.For(8, 2, 1).firstLayer, 138);  // urban
    EXPECT_EQ(materials.For(8, 2, 2).firstLayer, 144);  // suburban
}

TEST(Fs2024GroundMaterials, FieldsForestAndUnknownsFallBack) {
    const auto materials = f::Fs2024GroundMaterials::Read(WriteFixture());
    EXPECT_EQ(materials.For(1, 2).firstLayer, 272);   // cultivated: meadow
    EXPECT_EQ(materials.For(2, 2).firstLayer, 236);   // forest floor
    EXPECT_EQ(materials.For(3, 4).firstLayer, 10);    // any climate
    EXPECT_EQ(materials.For(99, 2).firstLayer, 10);   // unknown: grass
}

TEST(Fs2024GroundMaterials, RealIndexCoversEveryLondonClass) {
    const char* path = "D:/Games/Official/Steam/bf-texture-synth-lib/"
                       "TexSynthLibs/BFTexSynthLib/autogen/arrays.xml";
    if (!std::filesystem::exists(path)) GTEST_SKIP() << "no FS2024";
    const auto materials = f::Fs2024GroundMaterials::Read(path);
    // 184 biome entries, 9 of which name no array layers at all.
    EXPECT_EQ(materials.Count(), 175u);
    EXPECT_EQ(materials.For(8, 2, 1).firstLayer, 138);  // urban_su
    EXPECT_EQ(materials.For(3, 2).firstLayer, 10);      // tall_grass_heath
    for (int landClass : {1, 2, 3, 4, 5, 8, 9}) {       // all London has
        const auto material = materials.For(landClass, 2);
        EXPECT_GE(material.firstLayer, 0);
        EXPECT_LT(material.firstLayer + material.layers, 653);
    }
}

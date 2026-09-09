// Quake .skin files map a model's surfaces to textures. Player models
// rely on them: the shader names inside the MD3 are often absent, so
// trusting those leaves the model untextured.

#include "services/interfaces/workflow/quake3/q3_skin_file.hpp"

#include <gtest/gtest.h>

namespace q3 = sdl3cpp::q3;

namespace {
// models/players/keel/upper_default.skin, verbatim.
constexpr const char* kKeelUpper =
    "tag_head,\n"
    "tag_weapon,\n"
    "u_neck,models/players/Keel/keel_h.tga\n"
    "u_torso,models/players/Keel/keel.tga\n"
    "tag_torso,\n";
}  // namespace

TEST(ParseSkinFile, ReadsSurfaceToTexture) {
    const auto skin = q3::ParseSkinFile(kKeelUpper);
    EXPECT_EQ(q3::SkinTextureFor(skin, "u_torso"),
              "models/players/keel/keel.tga");
    EXPECT_EQ(q3::SkinTextureFor(skin, "u_neck"),
              "models/players/keel/keel_h.tga");
}

TEST(ParseSkinFile, SkipsTagEntriesWithNoTexture) {
    const auto skin = q3::ParseSkinFile(kKeelUpper);
    EXPECT_EQ(skin.size(), 2u);
    EXPECT_TRUE(q3::SkinTextureFor(skin, "tag_head").empty());
}

TEST(ParseSkinFile, LowercasesPathsBecauseThePk3IsExact) {
    // The shipped file says Keel; the stored entry is keel.
    const auto skin = q3::ParseSkinFile(kKeelUpper);
    EXPECT_EQ(q3::SkinTextureFor(skin, "u_torso").find("Keel"),
              std::string::npos);
}

TEST(ParseSkinFile, LooksUpSurfacesCaseInsensitively) {
    const auto skin = q3::ParseSkinFile(kKeelUpper);
    EXPECT_EQ(q3::SkinTextureFor(skin, "U_TORSO"),
              q3::SkinTextureFor(skin, "u_torso"));
}

TEST(ParseSkinFile, ToleratesBlankLinesAndCarriageReturns) {
    const auto skin = q3::ParseSkinFile("\r\n\nl_legs,a.tga\r\n\n");
    EXPECT_EQ(q3::SkinTextureFor(skin, "l_legs"), "a.tga");
}

TEST(ParseSkinFile, IgnoresLinesWithoutAComma) {
    const auto skin = q3::ParseSkinFile("garbage\nl_legs,a.tga\n");
    EXPECT_EQ(skin.size(), 1u);
}

TEST(ParseSkinFile, EmptyInputGivesEmptySkin) {
    EXPECT_TRUE(q3::ParseSkinFile("").empty());
    EXPECT_TRUE(q3::SkinTextureFor(q3::SkinMap{}, "anything").empty());
}

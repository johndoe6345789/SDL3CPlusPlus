// FS2024's own landmark models, placed the way the engine places them,
// against where their parts really stand. Skipped when the game is
// absent.

#include "services/interfaces/workflow/fs2024/data/bgl/fs2024_bgl_model_library.hpp"
#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_blocks.hpp"
#include "services/interfaces/workflow/fs2024/landmark/fs2024_landmark_place.hpp"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>

namespace f = sdl3cpp::fs2024;
namespace s = sdl3cpp::services::impl;

namespace {

constexpr const char* kLibrary =
    "D:/Games/Official/Steam/fs-base/scenery/Global/Asobo_POI/Asobo_POI.BGL";

f::GltfLod LoadLod(const std::string& name, std::size_t budget) {
    for (const auto& entry : f::ListModelLibrary(kLibrary)) {
        if (entry.name != name) continue;
        const auto riff = f::ReadModelRiff(kLibrary, entry);
        const auto lods = f::ListModelRiffLods(riff);
        for (const auto& lod : lods) {
            std::printf("  %s: %zu bytes, minSize %.1f\n",
                        lod.modelFile.c_str(), lod.bytes, lod.minSize);
        }
        return f::ParseModelRiffLod(riff,
                                    s::ChooseFs2024LandmarkLod(lods, budget));
    }
    return {};
}

}  // namespace

TEST(Fs2024LandmarkPlaceReal, WestminsterTowersStandWhereTheyReallyDo) {
    if (!std::filesystem::exists(kLibrary)) GTEST_SKIP() << "no FS2024";
    const f::GltfLod lod = LoadLod("WestminsterPalace", 8u << 20);
    const glm::mat4 model = s::Fs2024LandmarkModel(glm::vec3(0.f), 278.f, 1.f);
    // Everything above the roofs: the two towers, with the central spire.
    std::vector<glm::vec3> high;
    float top = 0.f;
    for (const auto& prim : lod.primitives) {
        for (const auto& v : prim.mesh.vertices) {
            const glm::vec3 p(model * glm::vec4(v.x, v.y, v.z, 1.f));
            top = std::max(top, p.y);
            if (p.y > 70.f) high.push_back(p);
        }
    }
    std::printf("top %.1f m, %zu high vertices\n", top, high.size());
    ASSERT_FALSE(high.empty());
    // North-most and south-most high parts: Elizabeth Tower (Big Ben)
    // and Victoria Tower, 258 m apart on a bearing of 191 degrees.
    const auto [north, south] = std::minmax_element(
        high.begin(), high.end(),
        [](const glm::vec3& a, const glm::vec3& b) { return a.z < b.z; });
    const glm::vec3 d = *south - *north;
    const double bearing =
        std::fmod(std::atan2(d.x, -d.z) * 180.0 / 3.14159265 + 360.0, 360.0);
    std::printf("north (%.0f, %.0f) south (%.0f, %.0f) bearing %.0f\n",
                north->x, north->z, south->x, south->z, bearing);
    EXPECT_NEAR(std::hypot(d.x, d.z), 258.0, 50.0);
    EXPECT_NEAR(bearing, 191.0, 20.0);
}

TEST(Fs2024LandmarkPlaceReal, ReadsTheColourMapsCompressedWithEveryMip) {
    const std::string texture =
        "D:/Games/Official/Steam/fs-base/scenery/Global/Asobo_POI/TEXTURE/"
        "PALACEOFWESTMINSTER_WALL02_ALBEDO.PNG.DDS";
    if (!std::filesystem::exists(texture)) GTEST_SKIP() << "no FS2024";
    const auto dds = f::ReadDdsBlocks(texture);
    EXPECT_EQ(dds.width, 2048u);
    EXPECT_EQ(dds.height, 2048u);
    EXPECT_EQ(dds.mips, 12u);
    // Every mip down to 1 x 1, each at least one block.
    std::size_t bytes = 0;
    for (std::uint32_t mip = 0; mip < dds.mips; ++mip) {
        const std::uint32_t side = std::max(2048u >> mip, 1u);
        bytes += std::size_t{(side + 3) / 4} * ((side + 3) / 4) *
                 dds.blockBytes;
    }
    EXPECT_EQ(dds.data.size(), bytes);
    EXPECT_EQ(std::filesystem::file_size(texture), 128 + bytes);
}

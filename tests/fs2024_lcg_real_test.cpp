// FS2024's ground-cover layer against the installed game. Skipped when
// the game is absent.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"
#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_lcg_tile.hpp"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <map>
#include <tuple>

namespace f = sdl3cpp::fs2024;

namespace {

constexpr const char* kLondonCover =
    "D:/Games/Official/Steam/fs-base-cgl/CGL/031/lcg313.cgl";

}  // namespace

TEST(Fs2024LcgReal, WestminsterTileDecodes) {
    if (!std::filesystem::exists(kLondonCover)) GTEST_SKIP() << "no FS2024";
    const auto cgl = f::ReadCglContainer(kLondonCover);
    // level 12 = six digits below the file's base key 031313: "131130"
    const std::uint32_t key = (6u << 12) | 0b01'11'01'11'01'00u;
    const f::CglTileEntry* entry = f::FindCglTile(cgl, key);
    ASSERT_NE(entry, nullptr);
    const f::Fs2024LcgImage image = f::DecodeFs2024LcgTile(f::ReadCglTile(cgl, *entry));
    std::map<std::tuple<int, int, int, int>, int> colours;
    for (int i = 0; i < image.width * image.height; ++i) {
        const std::uint8_t* p = image.rgba.data() + i * 4;
        ++colours[{p[0], p[1], p[2], p[3]}];
    }
    std::printf("%dx%d, %zu distinct values\n", image.width, image.height,
                colours.size());
    for (const auto& [c, n] : colours) {
        std::printf("  %3d %3d %3d %3d : %d\n", std::get<0>(c), std::get<1>(c),
                    std::get<2>(c), std::get<3>(c), n);
    }
    EXPECT_GE(image.width, 257);
}

TEST(Fs2024LcgReal, ValueCensusAcrossLondonFile) {
    if (!std::filesystem::exists(kLondonCover)) GTEST_SKIP() << "no FS2024";
    const auto cgl = f::ReadCglContainer(kLondonCover);
    std::map<int, long> greens;
    std::map<int, long> others;
    int decoded = 0;
    for (std::size_t i = 0; i < cgl.tiles.size(); i += 7) {
        if ((cgl.tiles[i].key >> 12) != 6) continue;  // level-12 tiles
        const auto image = f::DecodeFs2024LcgTile(f::ReadCglTile(cgl, cgl.tiles[i]));
        ++decoded;
        for (int p = 0; p < image.width * image.height; ++p) {
            const std::uint8_t* px = image.rgba.data() + p * 4;
            ++greens[px[1]];
            if (px[0] || px[2]) ++others[px[0] * 256 + px[2]];
        }
    }
    std::printf("%d level-12 tiles\n", decoded);
    for (const auto& [g, n] : greens) std::printf("  G %3d : %ld\n", g, n);
    std::printf("  pixels with R or B set: %zu distinct\n", others.size());
}

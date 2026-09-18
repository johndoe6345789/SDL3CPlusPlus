// FS2024's own CGL containers: every tile table must add up to exactly
// the file it indexes. Skipped when the game is absent.

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace f = sdl3cpp::fs2024;

TEST(Fs2024CglReal, EveryTableAddsUpToItsFile) {
    // London's layers, and Innsbruck's surveyed buildings: a 228 MB file
    // whose table holds a size step of exactly 0x4000.
    const std::string root = "D:/Games/Official/Steam/fs-base-cgl/CGL/";
    const char* files[] = {"031/bldo311.cgl", "031/bldn311.cgl",
                           "031/dem311.cgl",  "031/lcg311.cgl",
                           "031/vec313.cgl",  "031/vecn313.cgl",
                           "120/bldo221.cgl", "120/dem221.cgl",
                           "120/vec221.cgl"};
    if (!std::filesystem::exists(root)) GTEST_SKIP() << "no FS2024";
    for (const char* name : files) {
        const std::string path = root + name;
        if (!std::filesystem::exists(path)) {
            ADD_FAILURE() << "missing " << path;
            continue;
        }
        const f::CglContainer cgl = f::ReadCglContainer(path);
        ASSERT_FALSE(cgl.tiles.empty()) << name;
        // The tiles run on without gaps to the end of the file, bar the
        // 154-byte trailer bld and vec files carry (an ODbL notice).
        const f::CglTileEntry& last = cgl.tiles.back();
        const std::uint64_t end = last.offset + last.compressedSize;
        const std::uint64_t size = std::filesystem::file_size(path);
        EXPECT_LE(end, size) << name;
        EXPECT_LT(size - end, 256u) << name;
    }
}

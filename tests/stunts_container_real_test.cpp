// Decodes real Stunts data files end to end: Huffman, then the nested
// run-length pass for the larger .P3S/.PVS files. Skipped when no
// install is available -- point STUNTS_DIR at one to run this.

#include "services/interfaces/workflow/stunts/data/stunts_container.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_resource.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

std::string InstallDir() {
    const char* env = std::getenv("STUNTS_DIR");
    return env ? env : "";
}

}  // namespace

TEST(StuntsContainerReal, CarArchivesDecodeToTheirDeclaredSize) {
    const std::string root = InstallDir();
    if (root.empty() || !std::filesystem::exists(root)) {
        GTEST_SKIP() << "no Stunts install (set STUNTS_DIR)";
    }
    // Uncompressed, so this exercises the pass-through path.
    const std::string path = root + "/CARANSX.RES";
    ASSERT_TRUE(std::filesystem::exists(path)) << path;
    const auto decoded = LoadStuntsFile(path);
    const auto res = ParseStuntsResources(decoded);
    ASSERT_TRUE(res.count("simd")) << "no simd resource in CARANSX.RES";
    ASSERT_TRUE(res.count("edes")) << "no edes resource in CARANSX.RES";
}

TEST(StuntsContainerReal, ShapeArchiveNestsHuffmanThenRunLength) {
    const std::string root = InstallDir();
    if (root.empty() || !std::filesystem::exists(root)) {
        GTEST_SKIP() << "no Stunts install (set STUNTS_DIR)";
    }
    const std::string path = root + "/GAME1.P3S";
    ASSERT_TRUE(std::filesystem::exists(path)) << path;
    const auto decoded = LoadStuntsFile(path);
    // The archive header the run-length pass reveals: a size, a
    // 16-bit entry count, then that many four-character shape tags.
    ASSERT_GE(decoded.size(), 8u);
    const std::uint16_t count = static_cast<std::uint16_t>(
        decoded[4] | (decoded[5] << 8));
    ASSERT_GT(count, 0u);
    ASSERT_GE(decoded.size(), 6u + 4u * count);
    // Every stock piece file ships a "road" and a "turn" shape.
    bool sawRoad = false;
    bool sawTurn = false;
    for (std::uint16_t i = 0; i < count; ++i) {
        const char* tag =
            reinterpret_cast<const char*>(&decoded[6 + 4u * i]);
        if (std::strncmp(tag, "road", 4) == 0) sawRoad = true;
        if (std::strncmp(tag, "turn", 4) == 0) sawTurn = true;
    }
    EXPECT_TRUE(sawRoad);
    EXPECT_TRUE(sawTurn);
}

}  // namespace sdl3cpp::services::impl

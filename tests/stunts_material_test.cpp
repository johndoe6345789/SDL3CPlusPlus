#include "services/interfaces/workflow/stunts/data/stunts_material.hpp"

#include <gtest/gtest.h>

#include <cstdio>

using sdl3cpp::services::impl::LoadStuntsMaterialTable;
using sdl3cpp::services::impl::StuntsColorFor;

namespace {

/// Writes a small material table to a temp file and returns its path.
std::string WriteTempTable() {
    const std::string path = "stunts_material_test_tmp.json";
    std::FILE* file = std::fopen(path.c_str(), "w");
    std::fputs(R"({"default":"808080","colors":{"0":"000000",)"
              R"("19":"FCFCFC"}})",
              file);
    std::fclose(file);
    return path;
}

}  // namespace

TEST(StuntsMaterial, ReadsKnownColorsExactly) {
    const std::string path = WriteTempTable();
    const auto table = LoadStuntsMaterialTable(path);
    ASSERT_TRUE(table.loaded);
    EXPECT_EQ(StuntsColorFor(table, 0), glm::vec3(0.f));
    EXPECT_NEAR(StuntsColorFor(table, 19).r, 252.f / 255.f, 1e-5f);
    std::remove(path.c_str());
}

TEST(StuntsMaterial, FallsBackForAnUnknownId) {
    const std::string path = WriteTempTable();
    const auto table = LoadStuntsMaterialTable(path);
    ASSERT_TRUE(table.loaded);
    EXPECT_EQ(StuntsColorFor(table, 200), table.fallback);
    std::remove(path.c_str());
}

TEST(StuntsMaterial, MissingFileLeavesTheTableUnloaded) {
    const auto table = LoadStuntsMaterialTable("no_such_file.json");
    EXPECT_FALSE(table.loaded);
}

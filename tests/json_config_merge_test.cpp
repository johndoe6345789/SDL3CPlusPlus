#include <gtest/gtest.h>

#include "services/impl/config/json_config_service.hpp"
#include "services/interfaces/i_logger.hpp"

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <utility>

namespace {

class NullLogger final : public sdl3cpp::services::ILogger {
public:
    void SetLevel(sdl3cpp::services::LogLevel) override {}
    sdl3cpp::services::LogLevel GetLevel() const override { return sdl3cpp::services::LogLevel::OFF; }
    void SetOutputFile(const std::string&) override {}
    void SetMaxLinesPerFile(size_t) override {}
    void EnableConsoleOutput(bool) override {}
    void Log(sdl3cpp::services::LogLevel, const std::string&) override {}
    void Trace(const std::string&) override {}
    void Trace(const std::string&, const std::string&, const std::string&, const std::string&) override {}
    void Debug(const std::string&) override {}
    void Info(const std::string&) override {}
    void Warn(const std::string&) override {}
    void Error(const std::string&) override {}
    void TraceFunction(const std::string&) override {}
    void TraceVariable(const std::string&, const std::string&) override {}
    void TraceVariable(const std::string&, int) override {}
    void TraceVariable(const std::string&, size_t) override {}
    void TraceVariable(const std::string&, bool) override {}
    void TraceVariable(const std::string&, float) override {}
    void TraceVariable(const std::string&, double) override {}
};

class ScopedTempDir {
public:
    ScopedTempDir() {
        auto base = std::filesystem::temp_directory_path();
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = base / ("sdl3cpp_config_test_" + suffix);
        std::filesystem::create_directories(path_);
    }

    ~ScopedTempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path_, ec);
    }

    const std::filesystem::path& Path() const {
        return path_;
    }

private:
    std::filesystem::path path_;
};

std::filesystem::path GetRepoRoot() {
    return std::filesystem::path(__FILE__).parent_path().parent_path();
}

void WriteFile(const std::filesystem::path& path, const std::string& contents) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream output(path);
    ASSERT_TRUE(output.is_open()) << "Failed to open file for write: " << path;
    output << contents;
}

void CopyConfigAssets(const std::filesystem::path& targetDir) {
    const auto repoRoot = GetRepoRoot();
    const std::array<std::pair<std::filesystem::path, std::filesystem::path>, 2> assets = {
        std::make_pair(repoRoot / "config" / "schema" / "runtime_config_v2.schema.json",
                       targetDir / "schema" / "runtime_config_v2.schema.json"),
        std::make_pair(repoRoot / "config" / "workflows" / "templates" / "boot_default.json",
                       targetDir / "workflows" / "templates" / "boot_default.json")
    };
    for (const auto& asset : assets) {
        std::filesystem::create_directories(asset.second.parent_path());
        std::ifstream input(asset.first);
        ASSERT_TRUE(input.is_open()) << "Missing config asset: " << asset.first;
        std::ofstream output(asset.second);
        ASSERT_TRUE(output.is_open()) << "Failed to write config asset: " << asset.second;
        output << input.rdbuf();
    }
}


TEST(JsonConfigMergeTest, OverlayOverridesBaseFields) {
    ScopedTempDir tempDir;
    CopyConfigAssets(tempDir.Path());
    auto logger = std::make_shared<NullLogger>();

    const std::string baseConfig = R"({
  "schema_version": 2,
  "configVersion": 2,
  "window": { "size": { "width": 800, "height": 600 } }
})";
    const std::string overlayConfig = R"({
  "schema_version": 2,
  "configVersion": 2,
  "extends": "base.json",
  "window": { "size": { "width": 1024 } }
})";

    WriteFile(tempDir.Path() / "base.json", baseConfig);
    WriteFile(tempDir.Path() / "overlay.json", overlayConfig);

    sdl3cpp::services::impl::JsonConfigService configService(
        logger, tempDir.Path() / "overlay.json", false);

    const auto& config = configService.GetConfig();
    EXPECT_EQ(config.width, 1024u);
    EXPECT_EQ(config.height, 600u);
}

TEST(JsonConfigMergeTest, DeleteDirectiveRemovesObject) {
    ScopedTempDir tempDir;
    CopyConfigAssets(tempDir.Path());
    auto logger = std::make_shared<NullLogger>();

    const std::string baseConfig = R"({
  "schema_version": 2,
  "configVersion": 2,
  "window": {
    "title": "Base Title",
    "mouse_grab": { "enabled": true }
  }
})";
    const std::string overlayConfig = R"({
  "schema_version": 2,
  "configVersion": 2,
  "extends": "base.json",
  "window": {
    "@delete": ["mouse_grab"],
    "title": "Overlay Title"
  }
})";

    WriteFile(tempDir.Path() / "base.json", baseConfig);
    WriteFile(tempDir.Path() / "overlay.json", overlayConfig);

    sdl3cpp::services::impl::JsonConfigService configService(
        logger, tempDir.Path() / "overlay.json", false);

    const auto& config = configService.GetConfig();
    EXPECT_EQ(config.windowTitle, "Overlay Title");
    EXPECT_FALSE(config.mouseGrab.enabled);
}

TEST(JsonConfigMergeTest, ExtendsArrayAppliesInOrder) {
    ScopedTempDir tempDir;
    CopyConfigAssets(tempDir.Path());
    auto logger = std::make_shared<NullLogger>();

    const std::string baseOne = R"({
  "schema_version": 2,
  "configVersion": 2,
  "window": { "title": "Base One" }
})";
    const std::string baseTwo = R"({
  "schema_version": 2,
  "configVersion": 2,
  "window": { "title": "Base Two" }
})";
    const std::string overlayConfig = R"({
  "schema_version": 2,
  "configVersion": 2,
  "extends": ["base_one.json", "base_two.json"],
})";

    WriteFile(tempDir.Path() / "base_one.json", baseOne);
    WriteFile(tempDir.Path() / "base_two.json", baseTwo);
    WriteFile(tempDir.Path() / "overlay.json", overlayConfig);

    sdl3cpp::services::impl::JsonConfigService configService(
        logger, tempDir.Path() / "overlay.json", false);

    const auto& config = configService.GetConfig();
    EXPECT_EQ(config.windowTitle, "Base Two");
}

TEST(JsonConfigMergeTest, ExtendsCycleThrows) {
    ScopedTempDir tempDir;
    CopyConfigAssets(tempDir.Path());
    auto logger = std::make_shared<NullLogger>();

    const std::string baseA = R"({
  "schema_version": 2,
  "configVersion": 2,
  "extends": "base_b.json",
})";
    const std::string baseB = R"({
  "schema_version": 2,
  "configVersion": 2,
  "extends": "base_a.json",
})";

    WriteFile(tempDir.Path() / "base_a.json", baseA);
    WriteFile(tempDir.Path() / "base_b.json", baseB);

    EXPECT_THROW(
        sdl3cpp::services::impl::JsonConfigService(logger, tempDir.Path() / "base_a.json", false),
        std::runtime_error);
}

}  // namespace

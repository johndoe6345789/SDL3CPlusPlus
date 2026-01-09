#include <gtest/gtest.h>

#include "services/impl/json_config_service.hpp"
#include "services/interfaces/i_logger.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

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
        path_ = base / ("sdl3cpp_schema_test_" + suffix);
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

void CopySchema(const std::filesystem::path& targetDir) {
    auto schemaSource = GetRepoRoot() / "config" / "schema" / "runtime_config_v2.schema.json";
    auto schemaTarget = targetDir / "schema" / "runtime_config_v2.schema.json";
    std::filesystem::create_directories(schemaTarget.parent_path());
    std::ifstream input(schemaSource);
    ASSERT_TRUE(input.is_open()) << "Missing schema source: " << schemaSource;
    std::ofstream output(schemaTarget);
    ASSERT_TRUE(output.is_open()) << "Failed to write schema target: " << schemaTarget;
    output << input.rdbuf();
}

void WriteLuaScript(const std::filesystem::path& rootDir) {
    WriteFile(rootDir / "scripts" / "cube_logic.lua", "-- test script\n");
}

TEST(JsonConfigSchemaValidationTest, RejectsInvalidWindowWidthType) {
    ScopedTempDir tempDir;
    CopySchema(tempDir.Path());
    WriteLuaScript(tempDir.Path());
    auto logger = std::make_shared<NullLogger>();

    const std::string config = R"({
  "schema_version": 2,
  "configVersion": 2,
  "scripts": { "entry": "scripts/cube_logic.lua", "lua_debug": false },
  "window": { "size": { "width": "wide", "height": 600 } }
})";

    WriteFile(tempDir.Path() / "config.json", config);

    EXPECT_THROW(
        sdl3cpp::services::impl::JsonConfigService(logger, tempDir.Path() / "config.json", false),
        std::runtime_error);
}

TEST(JsonConfigSchemaValidationTest, RejectsInvalidSceneSourceEnum) {
    ScopedTempDir tempDir;
    CopySchema(tempDir.Path());
    WriteLuaScript(tempDir.Path());
    auto logger = std::make_shared<NullLogger>();

    const std::string config = R"({
  "schema_version": 2,
  "configVersion": 2,
  "scripts": { "entry": "scripts/cube_logic.lua", "lua_debug": false },
  "runtime": { "scene_source": "broken" }
})";

    WriteFile(tempDir.Path() / "config.json", config);

    EXPECT_THROW(
        sdl3cpp::services::impl::JsonConfigService(logger, tempDir.Path() / "config.json", false),
        std::runtime_error);
}

}  // namespace

#include <gtest/gtest.h>

#include "services/impl/graphics/bgfx_graphics_backend.hpp"
#include "services/interfaces/i_logger.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

bgfx::TextureHandle LoadTextureFromFileForTest(const BgfxGraphicsBackend& backend,
                                               const std::string& path,
                                               uint64_t samplerFlags) {
    return backend.LoadTextureFromFile(path, samplerFlags);
}

class CapturingLogger final : public sdl3cpp::services::ILogger {
public:
    void SetLevel(sdl3cpp::services::LogLevel level) override { level_ = level; }
    sdl3cpp::services::LogLevel GetLevel() const override { return level_; }
    void SetOutputFile(const std::string&) override {}
    void SetMaxLinesPerFile(size_t) override {}
    void EnableConsoleOutput(bool) override {}
    void Log(sdl3cpp::services::LogLevel level, const std::string& message) override {
        if (level == sdl3cpp::services::LogLevel::ERROR) {
            errors_.push_back(message);
        }
    }
    void Trace(const std::string&) override {}
    void Trace(const std::string&, const std::string&, const std::string&, const std::string&) override {}
    void Debug(const std::string&) override {}
    void Info(const std::string&) override {}
    void Warn(const std::string&) override {}
    void Error(const std::string& message) override { errors_.push_back(message); }
    void TraceFunction(const std::string&) override {}
    void TraceVariable(const std::string&, const std::string&) override {}
    void TraceVariable(const std::string&, int) override {}
    void TraceVariable(const std::string&, size_t) override {}
    void TraceVariable(const std::string&, bool) override {}
    void TraceVariable(const std::string&, float) override {}
    void TraceVariable(const std::string&, double) override {}

    bool HasErrorContaining(const std::string& needle) const {
        return std::any_of(errors_.begin(), errors_.end(),
                           [&needle](const std::string& message) {
                               return message.find(needle) != std::string::npos;
                           });
    }

private:
    sdl3cpp::services::LogLevel level_ = sdl3cpp::services::LogLevel::TRACE;
    std::vector<std::string> errors_;
};

TEST(BgfxGraphicsBackendFrameGuardTest, RejectsTextureLoadBeforeFirstFrame) {
    auto logger = std::make_shared<CapturingLogger>();
    BgfxGraphicsBackend backend(nullptr, nullptr, logger, nullptr);

    const bgfx::TextureHandle handle =
        LoadTextureFromFileForTest(backend, "missing_texture.png", BGFX_TEXTURE_NONE);

    EXPECT_FALSE(bgfx::isValid(handle));
    EXPECT_TRUE(logger->HasErrorContaining("Attempted to load texture BEFORE first bgfx::frame()"));
}

}  // namespace sdl3cpp::services::impl

#include <gtest/gtest.h>

#include "services/impl/graphics/bgfx_graphics_backend.hpp"
#include "services/interfaces/i_logger.hpp"

namespace sdl3cpp::services::impl {

bool CanAllocateTextureBudgetForTest(const BgfxGraphicsBackend& backend, size_t bytes) {
    return backend.textureMemoryTracker_.CanAllocate(bytes);
}

void AllocateTextureBudgetForTest(BgfxGraphicsBackend& backend, size_t bytes) {
    backend.textureMemoryTracker_.Allocate(bytes);
}

void FreeTextureBudgetForTest(BgfxGraphicsBackend& backend, size_t bytes) {
    backend.textureMemoryTracker_.Free(bytes);
}

void SetTextureBudgetMaxForTest(BgfxGraphicsBackend& backend, size_t maxBytes) {
    backend.textureMemoryTracker_.SetMaxBytes(maxBytes);
}

size_t GetTextureBudgetMaxForTest(const BgfxGraphicsBackend& backend) {
    return backend.textureMemoryTracker_.GetMaxBytes();
}

size_t GetTextureBudgetUsedForTest(const BgfxGraphicsBackend& backend) {
    return backend.textureMemoryTracker_.GetUsedBytes();
}

}  // namespace sdl3cpp::services::impl

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

class BudgetConfigService final : public sdl3cpp::services::IConfigService {
public:
    explicit BudgetConfigService(size_t vramMb) {
        budgets_.vramMB = vramMb;
    }

    uint32_t GetWindowWidth() const override { return 1; }
    uint32_t GetWindowHeight() const override { return 1; }
    std::filesystem::path GetScriptPath() const override { return {}; }
    bool IsLuaDebugEnabled() const override { return false; }
    std::string GetWindowTitle() const override { return ""; }
    sdl3cpp::services::SceneSource GetSceneSource() const override {
        return sdl3cpp::services::SceneSource::Lua;
    }
    const sdl3cpp::services::InputBindings& GetInputBindings() const override { return inputBindings_; }
    const sdl3cpp::services::MouseGrabConfig& GetMouseGrabConfig() const override { return mouseGrabConfig_; }
    const sdl3cpp::services::BgfxConfig& GetBgfxConfig() const override { return bgfxConfig_; }
    const sdl3cpp::services::MaterialXConfig& GetMaterialXConfig() const override { return materialXConfig_; }
    const std::vector<sdl3cpp::services::MaterialXMaterialConfig>& GetMaterialXMaterialConfigs() const override {
        return materialXMaterials_;
    }
    const sdl3cpp::services::GuiFontConfig& GetGuiFontConfig() const override { return guiFontConfig_; }
    const sdl3cpp::services::RenderBudgetConfig& GetRenderBudgetConfig() const override { return budgets_; }
    const sdl3cpp::services::CrashRecoveryConfig& GetCrashRecoveryConfig() const override { return crashRecovery_; }
    const sdl3cpp::services::ValidationTourConfig& GetValidationTourConfig() const override {
        return validationTour_;
    }
    const std::string& GetConfigJson() const override { return configJson_; }

private:
    sdl3cpp::services::InputBindings inputBindings_{};
    sdl3cpp::services::MouseGrabConfig mouseGrabConfig_{};
    sdl3cpp::services::BgfxConfig bgfxConfig_{};
    sdl3cpp::services::MaterialXConfig materialXConfig_{};
    std::vector<sdl3cpp::services::MaterialXMaterialConfig> materialXMaterials_{};
    sdl3cpp::services::GuiFontConfig guiFontConfig_{};
    sdl3cpp::services::RenderBudgetConfig budgets_{};
    sdl3cpp::services::CrashRecoveryConfig crashRecovery_{};
    sdl3cpp::services::ValidationTourConfig validationTour_{};
    std::string configJson_{};
};

TEST(BgfxTextureBudgetTrackerTest, UsesConfigBudgetForMaxBytes) {
    auto logger = std::make_shared<NullLogger>();
    auto configService = std::make_shared<BudgetConfigService>(2);
    sdl3cpp::services::impl::BgfxGraphicsBackend backend(
        configService, nullptr, logger, nullptr, nullptr);

    EXPECT_EQ(sdl3cpp::services::impl::GetTextureBudgetMaxForTest(backend), 2u * 1024u * 1024u);
}

TEST(BgfxTextureBudgetTrackerTest, TracksAllocationsAndFrees) {
    auto logger = std::make_shared<NullLogger>();
    auto configService = std::make_shared<BudgetConfigService>(1);
    sdl3cpp::services::impl::BgfxGraphicsBackend backend(
        configService, nullptr, logger, nullptr, nullptr);

    sdl3cpp::services::impl::SetTextureBudgetMaxForTest(backend, 100);
    EXPECT_TRUE(sdl3cpp::services::impl::CanAllocateTextureBudgetForTest(backend, 60));
    sdl3cpp::services::impl::AllocateTextureBudgetForTest(backend, 60);
    EXPECT_EQ(sdl3cpp::services::impl::GetTextureBudgetUsedForTest(backend), 60u);
    EXPECT_FALSE(sdl3cpp::services::impl::CanAllocateTextureBudgetForTest(backend, 50));

    sdl3cpp::services::impl::FreeTextureBudgetForTest(backend, 30);
    EXPECT_EQ(sdl3cpp::services::impl::GetTextureBudgetUsedForTest(backend), 30u);
    EXPECT_TRUE(sdl3cpp::services::impl::CanAllocateTextureBudgetForTest(backend, 50));
}

}  // namespace

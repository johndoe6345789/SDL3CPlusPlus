#include <gtest/gtest.h>

#include "services/impl/gui/bgfx_gui_service.hpp"
#include "services/interfaces/i_logger.hpp"

namespace sdl3cpp::services::impl {

void AddTextCacheEntryForTest(BgfxGuiService& service,
                              const std::string& text,
                              int fontSize,
                              uint64_t lastUsedFrame) {
    BgfxGuiService::TextKey key{text, fontSize};
    BgfxGuiService::TextTexture texture;
    texture.fontSize = fontSize;
    texture.lastUsedFrame = lastUsedFrame;
    service.textCache_[std::move(key)] = texture;
}

void AddSvgCacheEntryForTest(BgfxGuiService& service,
                             const std::string& path,
                             int width,
                             int height,
                             uint64_t lastUsedFrame) {
    BgfxGuiService::SvgKey key{path, width, height};
    BgfxGuiService::SvgTexture texture;
    texture.width = width;
    texture.height = height;
    texture.lastUsedFrame = lastUsedFrame;
    service.svgCache_[std::move(key)] = texture;
}

size_t GetTextCacheSizeForTest(const BgfxGuiService& service) {
    return service.textCache_.size();
}

size_t GetSvgCacheSizeForTest(const BgfxGuiService& service) {
    return service.svgCache_.size();
}

void PruneTextCacheForTest(BgfxGuiService& service) {
    service.PruneTextCache();
}

void PruneSvgCacheForTest(BgfxGuiService& service) {
    service.PruneSvgCache();
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
    BudgetConfigService(size_t textBudget, size_t svgBudget) {
        budgets_.guiTextCacheEntries = textBudget;
        budgets_.guiSvgCacheEntries = svgBudget;
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

TEST(BgfxGuiBudgetEnforcementTest, TextCachePrunesToBudget) {
    auto logger = std::make_shared<NullLogger>();
    auto configService = std::make_shared<BudgetConfigService>(2, 2);
    sdl3cpp::services::impl::BgfxGuiService service(configService, logger);

    sdl3cpp::services::impl::AddTextCacheEntryForTest(service, "a", 12, 1);
    sdl3cpp::services::impl::AddTextCacheEntryForTest(service, "b", 12, 2);
    sdl3cpp::services::impl::AddTextCacheEntryForTest(service, "c", 12, 3);

    sdl3cpp::services::impl::PruneTextCacheForTest(service);
    EXPECT_EQ(sdl3cpp::services::impl::GetTextCacheSizeForTest(service), 2u);
}

TEST(BgfxGuiBudgetEnforcementTest, SvgCachePrunesToBudget) {
    auto logger = std::make_shared<NullLogger>();
    auto configService = std::make_shared<BudgetConfigService>(2, 1);
    sdl3cpp::services::impl::BgfxGuiService service(configService, logger);

    sdl3cpp::services::impl::AddSvgCacheEntryForTest(service, "a.svg", 64, 64, 1);
    sdl3cpp::services::impl::AddSvgCacheEntryForTest(service, "b.svg", 64, 64, 2);

    sdl3cpp::services::impl::PruneSvgCacheForTest(service);
    EXPECT_EQ(sdl3cpp::services::impl::GetSvgCacheSizeForTest(service), 1u);
}

}  // namespace

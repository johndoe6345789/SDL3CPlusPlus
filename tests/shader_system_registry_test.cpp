#include <gtest/gtest.h>

#include "services/impl/config/config_compiler_service.hpp"
#include "services/impl/shader/shader_system_registry.hpp"

#include <string>

namespace {

class StubConfigService final : public sdl3cpp::services::IConfigService {
public:
    explicit StubConfigService(std::string json)
        : configJson_(std::move(json)) {}

    uint32_t GetWindowWidth() const override { return 0; }
    uint32_t GetWindowHeight() const override { return 0; }
    std::filesystem::path GetScriptPath() const override { return {}; }
    bool IsLuaDebugEnabled() const override { return false; }
    std::string GetWindowTitle() const override { return {}; }
    sdl3cpp::services::SceneSource GetSceneSource() const override {
        return sdl3cpp::services::SceneSource::Config;
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
    std::string configJson_;
};

TEST(ShaderSystemRegistryTest, UsesActiveGlslSystem) {
    const std::string json = R"({
  "shader_systems": {
    "active": "glsl"
  },
  "assets": {
    "shaders": {
      "flat": { "vs": "shaders/flat.vs", "fs": "shaders/flat.fs" }
    }
  }
})";

    auto configService = std::make_shared<StubConfigService>(json);
    auto configCompiler = std::make_shared<sdl3cpp::services::impl::ConfigCompilerService>(
        configService,
        nullptr,
        nullptr,
        nullptr);
    sdl3cpp::services::impl::ShaderSystemRegistry registry(
        configService,
        configCompiler,
        nullptr,
        nullptr);

    auto shaderMap = registry.BuildShaderMap();
    EXPECT_EQ(registry.GetActiveSystemId(), "glsl");
    auto reflection = registry.GetReflection("flat");
    EXPECT_TRUE(reflection.textures.empty());
    auto defaults = registry.GetDefaultTextures("flat");
    EXPECT_TRUE(defaults.empty());
    auto it = shaderMap.find("flat");
    ASSERT_NE(it, shaderMap.end());
    EXPECT_EQ(it->second.vertex, "shaders/flat.vs");
    EXPECT_EQ(it->second.fragment, "shaders/flat.fs");
}

TEST(ShaderSystemRegistryTest, FiltersShadersBySystem) {
    const std::string json = R"({
  "shader_systems": {
    "active": "glsl"
  },
  "assets": {
    "shaders": {
      "mx": { "vs": "shaders/mx.vs", "fs": "shaders/mx.fs", "system": "materialx" },
      "glsl": { "vs": "shaders/glsl.vs", "fs": "shaders/glsl.fs", "system": "glsl" },
      "default": { "vs": "shaders/default.vs", "fs": "shaders/default.fs" }
    }
  }
})";

    auto configService = std::make_shared<StubConfigService>(json);
    auto configCompiler = std::make_shared<sdl3cpp::services::impl::ConfigCompilerService>(
        configService,
        nullptr,
        nullptr,
        nullptr);
    sdl3cpp::services::impl::ShaderSystemRegistry registry(
        configService,
        configCompiler,
        nullptr,
        nullptr);

    auto shaderMap = registry.BuildShaderMap();
    EXPECT_EQ(shaderMap.size(), 2u);
    EXPECT_NE(shaderMap.find("glsl"), shaderMap.end());
    EXPECT_NE(shaderMap.find("default"), shaderMap.end());
    EXPECT_EQ(shaderMap.find("mx"), shaderMap.end());
    auto reflection = registry.GetReflection("glsl");
    EXPECT_TRUE(reflection.textures.empty());
}

}  // namespace

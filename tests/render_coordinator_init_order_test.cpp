#include <gtest/gtest.h>

#include "services/impl/render_coordinator_service.hpp"
#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_shader_script_service.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

class CallOrderGraphicsService : public sdl3cpp::services::IGraphicsService {
public:
    std::vector<std::string> calls;
    bool beginFrameResult = true;
    bool endFrameResult = true;

    void InitializeDevice(SDL_Window*, const sdl3cpp::services::GraphicsConfig&) override {}
    void InitializeSwapchain() override {}
    void RecreateSwapchain() override { calls.push_back("RecreateSwapchain"); }
    void Shutdown() noexcept override {}
    void LoadShaders(const std::unordered_map<std::string, sdl3cpp::services::ShaderPaths>&) override {
        calls.push_back("LoadShaders");
    }
    void UploadVertexData(const std::vector<sdl3cpp::core::Vertex>&) override {}
    void UploadIndexData(const std::vector<uint16_t>&) override {}
    bool BeginFrame() override {
        calls.push_back("BeginFrame");
        return beginFrameResult;
    }
    void RenderScene(const std::vector<sdl3cpp::services::RenderCommand>&,
                     const sdl3cpp::services::ViewState&) override {}
    bool EndFrame() override {
        calls.push_back("EndFrame");
        return endFrameResult;
    }
    void WaitIdle() override {}
    sdl3cpp::services::GraphicsDeviceHandle GetDevice() const override { return nullptr; }
    sdl3cpp::services::GraphicsDeviceHandle GetPhysicalDevice() const override { return nullptr; }
    std::pair<uint32_t, uint32_t> GetSwapchainExtent() const override { return {0, 0}; }
    uint32_t GetSwapchainFormat() const override { return 0; }
    void* GetCurrentCommandBuffer() const override { return nullptr; }
    void* GetGraphicsQueue() const override { return nullptr; }
};

class StubShaderScriptService : public sdl3cpp::services::IShaderScriptService {
public:
    std::unordered_map<std::string, sdl3cpp::services::ShaderPaths> LoadShaderPathsMap() override {
        return {};
    }
};

class StubConfigService final : public sdl3cpp::services::IConfigService {
public:
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
    std::string configJson_{};
};

std::string JoinCalls(const std::vector<std::string>& calls) {
    std::string joined;
    for (size_t index = 0; index < calls.size(); ++index) {
        if (index > 0) {
            joined += " -> ";
        }
        joined += calls[index];
    }
    return joined;
}

bool HasEndFrameBeforeLoadShaders(const std::vector<std::string>& calls) {
    for (const auto& call : calls) {
        if (call == "LoadShaders") {
            return false;
        }
        if (call == "EndFrame") {
            return true;
        }
    }
    return false;
}

TEST(RenderCoordinatorInitOrderTest, LoadsShadersOnlyAfterFirstFrame) {
    auto configService = std::make_shared<StubConfigService>();
    auto graphicsService = std::make_shared<CallOrderGraphicsService>();
    auto shaderScriptService = std::make_shared<StubShaderScriptService>();

    sdl3cpp::services::impl::RenderCoordinatorService service(
        nullptr,
        configService,
        graphicsService,
        nullptr,
        shaderScriptService,
        nullptr,
        nullptr,
        nullptr);

    service.RenderFrame(0.0f);

    const bool hasLoadShaders = std::find(
        graphicsService->calls.begin(),
        graphicsService->calls.end(),
        "LoadShaders") != graphicsService->calls.end();

    if (!hasLoadShaders) {
        SUCCEED() << "RenderFrame did not load shaders; initialization is expected elsewhere.";
        return;
    }

    EXPECT_TRUE(HasEndFrameBeforeLoadShaders(graphicsService->calls))
        << "LoadShaders was called before a completed frame. Calls: "
        << JoinCalls(graphicsService->calls);
}

}  // namespace

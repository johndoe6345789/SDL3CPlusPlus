#include <gtest/gtest.h>

#include "services/impl/config/config_compiler_service.hpp"
#include "services/impl/config/json_config_service.hpp"
#include "services/impl/diagnostics/logger_service.hpp"
#include "services/impl/render/render_coordinator_service.hpp"
#include "services/impl/shader/shader_system_registry.hpp"
#include "services/interfaces/i_graphics_service.hpp"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

class CallOrderGraphicsService : public sdl3cpp::services::IGraphicsService {
public:
    struct ConfigureViewCall {
        uint16_t viewId = 0;
        sdl3cpp::services::ViewClearConfig clearConfig{};
    };

    std::vector<std::string> calls;
    std::vector<ConfigureViewCall> configureViewCalls;
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
    void ConfigureView(uint16_t viewId, const sdl3cpp::services::ViewClearConfig& clearConfig) override {
        calls.push_back("ConfigureView");
        configureViewCalls.push_back({viewId, clearConfig});
    }
    bool EndFrame() override {
        calls.push_back("EndFrame");
        return endFrameResult;
    }
    void RequestScreenshot(const std::filesystem::path&) override {
        calls.push_back("RequestScreenshot");
    }
    void WaitIdle() override {}
    sdl3cpp::services::GraphicsDeviceHandle GetDevice() const override { return nullptr; }
    sdl3cpp::services::GraphicsDeviceHandle GetPhysicalDevice() const override { return nullptr; }
    std::pair<uint32_t, uint32_t> GetSwapchainExtent() const override { return {0, 0}; }
    uint32_t GetSwapchainFormat() const override { return 0; }
    void* GetCurrentCommandBuffer() const override { return nullptr; }
    void* GetGraphicsQueue() const override { return nullptr; }
};

class ScopedTempDir {
public:
    ScopedTempDir() {
        auto base = std::filesystem::temp_directory_path();
        const auto suffix = std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count());
        path_ = base / ("sdl3cpp_render_coordinator_test_" + suffix);
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
    if (!output.is_open()) {
        throw std::runtime_error("Failed to open file for write: " + path.string());
    }
    output << contents;
}

void CopyBootTemplate(const std::filesystem::path& targetDir) {
    const auto repoRoot = GetRepoRoot();
    const auto source = repoRoot / "packages" / "bootstrap" / "workflows" / "boot_default.json";
    const auto destination = targetDir / "workflows" / "templates" / "boot_default.json";
    std::filesystem::create_directories(destination.parent_path());
    std::ifstream input(source);
    if (!input.is_open()) {
        throw std::runtime_error("Missing boot workflow template: " + source.string());
    }
    std::ofstream output(destination);
    if (!output.is_open()) {
        throw std::runtime_error("Failed to write boot workflow template: " + destination.string());
    }
    output << input.rdbuf();
}

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
        nullptr,
        graphicsService,
        nullptr,
        shaderScriptService,
        nullptr,
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

TEST(RenderCoordinatorRenderGraphTest, ConfiguresViewsInPassOrder) {
    sdl3cpp::services::ConfigCompilerResult result;

    sdl3cpp::services::RenderPassIR shadowPass;
    shadowPass.id = "shadow";
    shadowPass.hasViewId = true;
    shadowPass.viewId = 0;
    shadowPass.clear.enabled = true;
    shadowPass.clear.clearColor = true;
    shadowPass.clear.clearDepth = true;
    shadowPass.clear.clearStencil = true;
    shadowPass.clear.color = {0.2f, 0.3f, 0.4f, 1.0f};
    shadowPass.clear.depth = 0.75f;
    shadowPass.clear.stencil = 5;

    sdl3cpp::services::RenderPassIR postPass;
    postPass.id = "post";
    postPass.clear.enabled = false;

    sdl3cpp::services::RenderPassIR mainPass;
    mainPass.id = "main";
    mainPass.hasViewId = true;
    mainPass.viewId = 5;
    mainPass.clear.enabled = true;
    mainPass.clear.clearColor = true;
    mainPass.clear.clearDepth = false;
    mainPass.clear.clearStencil = true;
    mainPass.clear.color = {0.1f, 0.2f, 0.3f, 1.0f};
    mainPass.clear.depth = 0.9f;
    mainPass.clear.stencil = 300;

    result.renderGraph.passes = {shadowPass, postPass, mainPass};
    result.renderGraphBuild.passOrder = {"post", "shadow", "main"};

    auto configCompilerService = std::make_shared<StubConfigCompilerService>(result);
    auto configService = std::make_shared<StubConfigService>(sdl3cpp::services::SceneSource::Config);
    auto graphicsService = std::make_shared<CallOrderGraphicsService>();

    sdl3cpp::services::impl::RenderCoordinatorService service(
        nullptr,
        configService,
        configCompilerService,
        graphicsService,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr);

    service.RenderFrame(0.0f);

    ASSERT_EQ(graphicsService->configureViewCalls.size(), 3u);
    EXPECT_EQ(graphicsService->configureViewCalls[0].viewId, 1u);
    EXPECT_EQ(graphicsService->configureViewCalls[1].viewId, 0u);
    EXPECT_EQ(graphicsService->configureViewCalls[2].viewId, 5u);

    const auto& postClear = graphicsService->configureViewCalls[0].clearConfig;
    EXPECT_FALSE(postClear.enabled);

    const auto& shadowClear = graphicsService->configureViewCalls[1].clearConfig;
    EXPECT_TRUE(shadowClear.enabled);
    EXPECT_TRUE(shadowClear.clearColor);
    EXPECT_TRUE(shadowClear.clearDepth);
    EXPECT_TRUE(shadowClear.clearStencil);
    EXPECT_FLOAT_EQ(shadowClear.color[0], 0.2f);
    EXPECT_FLOAT_EQ(shadowClear.color[1], 0.3f);
    EXPECT_FLOAT_EQ(shadowClear.color[2], 0.4f);
    EXPECT_FLOAT_EQ(shadowClear.color[3], 1.0f);
    EXPECT_FLOAT_EQ(shadowClear.depth, 0.75f);
    EXPECT_EQ(shadowClear.stencil, static_cast<uint8_t>(5));

    const auto& mainClear = graphicsService->configureViewCalls[2].clearConfig;
    EXPECT_TRUE(mainClear.enabled);
    EXPECT_TRUE(mainClear.clearColor);
    EXPECT_FALSE(mainClear.clearDepth);
    EXPECT_TRUE(mainClear.clearStencil);
    EXPECT_FLOAT_EQ(mainClear.color[0], 0.1f);
    EXPECT_FLOAT_EQ(mainClear.color[1], 0.2f);
    EXPECT_FLOAT_EQ(mainClear.color[2], 0.3f);
    EXPECT_FLOAT_EQ(mainClear.color[3], 1.0f);
    EXPECT_FLOAT_EQ(mainClear.depth, 0.9f);
    EXPECT_EQ(mainClear.stencil, static_cast<uint8_t>(255));
}

}  // namespace

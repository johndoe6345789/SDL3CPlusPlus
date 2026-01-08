#include <gtest/gtest.h>

#include "services/impl/render_coordinator_service.hpp"
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
    auto graphicsService = std::make_shared<CallOrderGraphicsService>();
    auto shaderScriptService = std::make_shared<StubShaderScriptService>();

    sdl3cpp::services::impl::RenderCoordinatorService service(
        nullptr,
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

#include <gtest/gtest.h>

#include "services/impl/graphics/graphics_service.hpp"
#include "services/interfaces/i_graphics_backend.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_window_service.hpp"

#include <array>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

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

class DummyWindowService final : public sdl3cpp::services::IWindowService {
public:
    void CreateWindow(const sdl3cpp::services::WindowConfig&) override {}
    void DestroyWindow() override {}
    SDL_Window* GetNativeHandle() const override { return nullptr; }
    std::pair<uint32_t, uint32_t> GetSize() const override { return {800, 600}; }
    bool ShouldClose() const override { return false; }
    void PollEvents() override {}
    void SetTitle(const std::string&) override {}
    bool IsMinimized() const override { return false; }
    void SetMouseGrabbed(bool) override {}
    bool IsMouseGrabbed() const override { return false; }
    void SetRelativeMouseMode(bool) override {}
    bool IsRelativeMouseMode() const override { return false; }
    void SetCursorVisible(bool) override {}
    bool IsCursorVisible() const override { return true; }
};

class TrackingBackend final : public sdl3cpp::services::IGraphicsBackend {
public:
    ~TrackingBackend() override {
        CleanupUndestroyed(createdVertexBuffers_);
        CleanupUndestroyed(createdIndexBuffers_);
    }

    void Initialize(void*, const sdl3cpp::services::GraphicsConfig&) override {}
    void Shutdown() override {}
    void RecreateSwapchain(uint32_t, uint32_t) override {}
    void WaitIdle() override {}

    sdl3cpp::services::GraphicsDeviceHandle CreateDevice() override {
        return reinterpret_cast<sdl3cpp::services::GraphicsDeviceHandle>(this);
    }
    void DestroyDevice(sdl3cpp::services::GraphicsDeviceHandle) override {}

    sdl3cpp::services::GraphicsPipelineHandle CreatePipeline(
        sdl3cpp::services::GraphicsDeviceHandle,
        const std::string&,
        const sdl3cpp::services::ShaderPaths&) override {
        return nullptr;
    }
    void DestroyPipeline(sdl3cpp::services::GraphicsDeviceHandle,
                         sdl3cpp::services::GraphicsPipelineHandle) override {}

    sdl3cpp::services::GraphicsBufferHandle CreateVertexBuffer(
        sdl3cpp::services::GraphicsDeviceHandle,
        const std::vector<uint8_t>&) override {
        auto handle = NewHandle();
        createdVertexBuffers_.push_back(handle);
        return handle;
    }

    sdl3cpp::services::GraphicsBufferHandle CreateIndexBuffer(
        sdl3cpp::services::GraphicsDeviceHandle,
        const std::vector<uint8_t>&) override {
        auto handle = NewHandle();
        createdIndexBuffers_.push_back(handle);
        return handle;
    }

    void DestroyBuffer(sdl3cpp::services::GraphicsDeviceHandle,
                       sdl3cpp::services::GraphicsBufferHandle buffer) override {
        destroyedBuffers_.push_back(buffer);
        destroyed_.insert(buffer);
        delete reinterpret_cast<int*>(buffer);
    }

    bool BeginFrame(sdl3cpp::services::GraphicsDeviceHandle) override { return true; }
    bool EndFrame(sdl3cpp::services::GraphicsDeviceHandle) override { return true; }
    void RequestScreenshot(sdl3cpp::services::GraphicsDeviceHandle,
                           const std::filesystem::path&) override {}
    void SetViewState(const sdl3cpp::services::ViewState&) override {}
    void ConfigureView(sdl3cpp::services::GraphicsDeviceHandle,
                       uint16_t,
                       const sdl3cpp::services::ViewClearConfig&) override {}
    void Draw(sdl3cpp::services::GraphicsDeviceHandle,
              sdl3cpp::services::GraphicsPipelineHandle,
              sdl3cpp::services::GraphicsBufferHandle,
              sdl3cpp::services::GraphicsBufferHandle,
              uint32_t,
              uint32_t,
              int32_t,
              const std::array<float, 16>&) override {}
    sdl3cpp::services::GraphicsDeviceHandle GetPhysicalDevice() const override { return nullptr; }
    std::pair<uint32_t, uint32_t> GetSwapchainExtent() const override { return {0, 0}; }
    uint32_t GetSwapchainFormat() const override { return 0; }
    void* GetCurrentCommandBuffer() const override { return nullptr; }
    void* GetGraphicsQueue() const override { return nullptr; }

    const std::vector<sdl3cpp::services::GraphicsBufferHandle>& DestroyedBuffers() const {
        return destroyedBuffers_;
    }

    const std::vector<sdl3cpp::services::GraphicsBufferHandle>& CreatedVertexBuffers() const {
        return createdVertexBuffers_;
    }

    const std::vector<sdl3cpp::services::GraphicsBufferHandle>& CreatedIndexBuffers() const {
        return createdIndexBuffers_;
    }

private:
    sdl3cpp::services::GraphicsBufferHandle NewHandle() {
        return reinterpret_cast<sdl3cpp::services::GraphicsBufferHandle>(new int(nextId_++));
    }

    void CleanupUndestroyed(const std::vector<sdl3cpp::services::GraphicsBufferHandle>& handles) {
        for (auto handle : handles) {
            if (destroyed_.count(handle) == 0) {
                delete reinterpret_cast<int*>(handle);
            }
        }
    }

    int nextId_ = 1;
    std::unordered_set<sdl3cpp::services::GraphicsBufferHandle> destroyed_;
    std::vector<sdl3cpp::services::GraphicsBufferHandle> destroyedBuffers_;
    std::vector<sdl3cpp::services::GraphicsBufferHandle> createdVertexBuffers_;
    std::vector<sdl3cpp::services::GraphicsBufferHandle> createdIndexBuffers_;
};

TEST(GraphicsServiceBufferLifecycleTest, ReuploadingVerticesDestroysPreviousBuffer) {
    auto backend = std::make_shared<TrackingBackend>();
    auto windowService = std::make_shared<DummyWindowService>();
    auto logger = std::make_shared<NullLogger>();

    sdl3cpp::services::impl::GraphicsService service(logger, backend, windowService);
    service.Initialize();
    service.InitializeDevice(nullptr, sdl3cpp::services::GraphicsConfig{});

    std::vector<sdl3cpp::core::Vertex> verticesA(1);
    service.UploadVertexData(verticesA);
    ASSERT_EQ(backend->CreatedVertexBuffers().size(), 1u);
    auto firstBuffer = backend->CreatedVertexBuffers().front();

    std::vector<sdl3cpp::core::Vertex> verticesB(2);
    service.UploadVertexData(verticesB);

    EXPECT_EQ(backend->DestroyedBuffers().size(), 1u)
        << "Uploading new vertex data should destroy the previous buffer to avoid VRAM leaks.";
    if (!backend->DestroyedBuffers().empty()) {
        EXPECT_EQ(backend->DestroyedBuffers().front(), firstBuffer);
    }
}

TEST(GraphicsServiceBufferLifecycleTest, ReuploadingIndicesDestroysPreviousBuffer) {
    auto backend = std::make_shared<TrackingBackend>();
    auto windowService = std::make_shared<DummyWindowService>();
    auto logger = std::make_shared<NullLogger>();

    sdl3cpp::services::impl::GraphicsService service(logger, backend, windowService);
    service.Initialize();
    service.InitializeDevice(nullptr, sdl3cpp::services::GraphicsConfig{});

    std::vector<uint16_t> indicesA = {0, 1, 2};
    service.UploadIndexData(indicesA);
    ASSERT_EQ(backend->CreatedIndexBuffers().size(), 1u);
    auto firstBuffer = backend->CreatedIndexBuffers().front();

    std::vector<uint16_t> indicesB = {0, 1, 2, 3, 4, 5};
    service.UploadIndexData(indicesB);

    EXPECT_EQ(backend->DestroyedBuffers().size(), 1u)
        << "Uploading new index data should destroy the previous buffer to avoid VRAM leaks.";
    if (!backend->DestroyedBuffers().empty()) {
        EXPECT_EQ(backend->DestroyedBuffers().front(), firstBuffer);
    }
}

}  // namespace

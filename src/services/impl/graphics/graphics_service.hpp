#pragma once

#include "services/interfaces/i_graphics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_window_service.hpp"
#include "../../../di/lifecycle.hpp"
#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief Graphics service implementation.
 *
 * Coordinates graphics backend operations.
 */
class GraphicsService : public IGraphicsService,
                        public di::IInitializable,
                        public di::IShutdownable {
public:
    GraphicsService(std::shared_ptr<ILogger> logger,
                    std::shared_ptr<IGraphicsBackend> backend,
                    std::shared_ptr<IWindowService> windowService);
    ~GraphicsService() override;

    // IInitializable interface
    void Initialize() override;

    // IShutdownable interface
    void Shutdown() noexcept override;

    // IGraphicsService interface
    void InitializeDevice(SDL_Window* window, const GraphicsConfig& config) override;
    void InitializeSwapchain() override;
    void RecreateSwapchain() override;
    void LoadShaders(const std::unordered_map<std::string, ShaderPaths>& shaders) override;
    void UploadVertexData(const std::vector<core::Vertex>& vertices) override;
    void UploadIndexData(const std::vector<uint16_t>& indices) override;
    bool BeginFrame() override;
    void RenderScene(const std::vector<RenderCommand>& commands,
                     const ViewState& viewState) override;
    void ConfigureView(uint16_t viewId, const ViewClearConfig& clearConfig) override;
    bool EndFrame() override;
    void RequestScreenshot(const std::filesystem::path& outputPath) override;
    void WaitIdle() override;
    GraphicsDeviceHandle GetDevice() const override;
    GraphicsDeviceHandle GetPhysicalDevice() const override;
    std::pair<uint32_t, uint32_t> GetSwapchainExtent() const override;
    uint32_t GetSwapchainFormat() const override;
    void* GetCurrentCommandBuffer() const override;
    void* GetGraphicsQueue() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IGraphicsBackend> backend_;
    std::shared_ptr<IWindowService> windowService_;
    GraphicsDeviceHandle device_ = nullptr;
    std::unordered_map<std::string, GraphicsPipelineHandle> pipelines_;
    GraphicsBufferHandle vertexBuffer_ = nullptr;
    GraphicsBufferHandle indexBuffer_ = nullptr;
    // Other state
    bool initialized_ = false;
};

} // namespace sdl3cpp::services::impl

#pragma once

#include "../interfaces/i_graphics_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../interfaces/i_swapchain_service.hpp"
#include "../interfaces/i_pipeline_service.hpp"
#include "../interfaces/i_buffer_service.hpp"
#include "../interfaces/i_render_command_service.hpp"
#include "../interfaces/i_window_service.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Graphics service implementation.
 *
 * Coordinates all graphics subsystems (device, swapchain, pipeline, buffers, rendering).
 * Acts as a facade for the smaller graphics services.
 */
class GraphicsService : public IGraphicsService,
                        public di::IInitializable,
                        public di::IShutdownable {
public:
    GraphicsService(std::shared_ptr<ILogger> logger,
                    std::shared_ptr<IVulkanDeviceService> deviceService,
                    std::shared_ptr<ISwapchainService> swapchainService,
                    std::shared_ptr<IPipelineService> pipelineService,
                    std::shared_ptr<IBufferService> bufferService,
                    std::shared_ptr<IRenderCommandService> renderCommandService,
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
                    const std::array<float, 16>& viewProj) override;
    bool EndFrame() override;
    void WaitIdle() override;
    VkDevice GetDevice() const override;
    VkPhysicalDevice GetPhysicalDevice() const override;
    VkExtent2D GetSwapchainExtent() const override;
    VkFormat GetSwapchainFormat() const override;
    VkCommandBuffer GetCurrentCommandBuffer() const override;
    VkQueue GetGraphicsQueue() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IVulkanDeviceService> deviceService_;
    std::shared_ptr<ISwapchainService> swapchainService_;
    std::shared_ptr<IPipelineService> pipelineService_;
    std::shared_ptr<IBufferService> bufferService_;
    std::shared_ptr<IRenderCommandService> renderCommandService_;
    std::shared_ptr<IWindowService> windowService_;
    bool initialized_ = false;
    uint32_t currentImageIndex_ = 0;
};

}  // namespace sdl3cpp::services::impl
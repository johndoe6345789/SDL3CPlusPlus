#pragma once

#include "../interfaces/i_graphics_backend.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../interfaces/i_swapchain_service.hpp"
#include "../interfaces/i_render_command_service.hpp"
#include "../interfaces/i_pipeline_service.hpp"
#include "../interfaces/i_buffer_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/graphics_types.hpp"
#include <vulkan/vulkan.h>
#include <memory>
#include <unordered_map>

namespace sdl3cpp::services::impl {

/**
 * @brief Vulkan implementation of the graphics backend.
 *
 * Uses existing Vulkan services for device, swapchain, commands, pipelines, and buffers.
 */
class VulkanGraphicsBackend : public IGraphicsBackend {
public:
    VulkanGraphicsBackend(std::shared_ptr<IVulkanDeviceService> deviceService,
                         std::shared_ptr<ISwapchainService> swapchainService,
                         std::shared_ptr<IRenderCommandService> renderCommandService,
                         std::shared_ptr<IPipelineService> pipelineService,
                         std::shared_ptr<IBufferService> bufferService,
                         std::shared_ptr<ILogger> logger);
    ~VulkanGraphicsBackend() override;

    void Initialize(void* window, const GraphicsConfig& config) override;
    void Shutdown() override;

    GraphicsDeviceHandle CreateDevice() override;
    void DestroyDevice(GraphicsDeviceHandle device) override;

    GraphicsPipelineHandle CreatePipeline(GraphicsDeviceHandle device, const std::string& shaderKey, const ShaderPaths& shaderPaths) override;
    void DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) override;

    GraphicsBufferHandle CreateVertexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) override;
    GraphicsBufferHandle CreateIndexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) override;
    void DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) override;

    bool BeginFrame(GraphicsDeviceHandle device) override;
    bool EndFrame(GraphicsDeviceHandle device) override;

    void SetViewProjection(const std::array<float, 16>& viewProj) override;

    void Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
              GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
              uint32_t indexCount, const std::array<float, 16>& modelMatrix) override;

private:
    std::shared_ptr<IVulkanDeviceService> deviceService_;
    std::shared_ptr<ISwapchainService> swapchainService_;
    std::shared_ptr<IRenderCommandService> renderCommandService_;
    std::shared_ptr<IPipelineService> pipelineService_;
    std::shared_ptr<IBufferService> bufferService_;
    std::shared_ptr<ILogger> logger_;

    SDL_Window* window_;
    bool initialized_;
    uint32_t currentImageIndex_;
    std::vector<RenderCommand> frameCommands_;
    std::array<float, 16> currentViewProj_;
    std::unordered_map<GraphicsPipelineHandle, std::string> pipelineToShaderKey_;
};

}  // namespace sdl3cpp::services::impl
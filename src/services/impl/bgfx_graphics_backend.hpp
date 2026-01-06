#pragma once

#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_graphics_backend.hpp"
#include "../interfaces/i_logger.hpp"
#include "../interfaces/i_platform_service.hpp"
#include "../../core/vertex.hpp"
#include <bgfx/bgfx.h>
#include <array>
#include <memory>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

class BgfxGraphicsBackend : public IGraphicsBackend {
public:
    BgfxGraphicsBackend(std::shared_ptr<IConfigService> configService,
                        std::shared_ptr<IPlatformService> platformService,
                        std::shared_ptr<ILogger> logger);
    ~BgfxGraphicsBackend() override;

    void Initialize(void* window, const GraphicsConfig& config) override;
    void Shutdown() override;
    void RecreateSwapchain(uint32_t width, uint32_t height) override;
    void WaitIdle() override;
    GraphicsDeviceHandle CreateDevice() override;
    void DestroyDevice(GraphicsDeviceHandle device) override;
    GraphicsPipelineHandle CreatePipeline(GraphicsDeviceHandle device,
                                          const std::string& shaderKey,
                                          const ShaderPaths& shaderPaths) override;
    void DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) override;
    GraphicsBufferHandle CreateVertexBuffer(GraphicsDeviceHandle device,
                                            const std::vector<uint8_t>& data) override;
    GraphicsBufferHandle CreateIndexBuffer(GraphicsDeviceHandle device,
                                           const std::vector<uint8_t>& data) override;
    void DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) override;
    bool BeginFrame(GraphicsDeviceHandle device) override;
    bool EndFrame(GraphicsDeviceHandle device) override;
    void SetViewState(const ViewState& viewState) override;
    void Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
              GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
              uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset,
              const std::array<float, 16>& modelMatrix) override;
    GraphicsDeviceHandle GetPhysicalDevice() const override;
    std::pair<uint32_t, uint32_t> GetSwapchainExtent() const override;
    uint32_t GetSwapchainFormat() const override;
    void* GetCurrentCommandBuffer() const override;
    void* GetGraphicsQueue() const override;

private:
    struct PipelineEntry {
        bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
    };

    struct VertexBufferEntry {
        bgfx::VertexBufferHandle handle = BGFX_INVALID_HANDLE;
        uint32_t vertexCount = 0;
    };

    struct IndexBufferEntry {
        bgfx::IndexBufferHandle handle = BGFX_INVALID_HANDLE;
        uint32_t indexCount = 0;
    };

    struct MaterialXUniforms {
        bgfx::UniformHandle worldMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle viewMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle projectionMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle viewProjectionMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle worldViewMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle worldViewProjectionMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle worldInverseTransposeMatrix = BGFX_INVALID_HANDLE;
        bgfx::UniformHandle viewPosition = BGFX_INVALID_HANDLE;
    };

    struct PlatformHandleInfo {
        bool hasWayland = false;
        bool hasX11 = false;
        bool hasWindowHandle = false;
        bool hasDisplayHandle = false;
        bgfx::NativeWindowHandleType::Enum handleType = bgfx::NativeWindowHandleType::Default;
    };

    void SetupPlatformData(void* window);
    bgfx::RendererType::Enum ResolveRendererType() const;
    void LogRendererFailureDetails(bgfx::RendererType::Enum renderer,
                                   const std::vector<bgfx::RendererType::Enum>& supportedRenderers,
                                   const std::string& platformName,
                                   const std::string& videoDriverName);
    std::vector<uint8_t> ReadShaderSource(const std::string& path,
                                          const std::string& source) const;
    bgfx::ShaderHandle CreateShader(const std::string& label,
                                    const std::string& source,
                                    bool isVertex) const;
    void InitializeUniforms();
    void DestroyUniforms();
    void ApplyMaterialXUniforms(const std::array<float, 16>& modelMatrix);
    void DestroyPipelines();
    void DestroyBuffers();

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IPlatformService> platformService_;
    std::shared_ptr<ILogger> logger_;
    bgfx::VertexLayout vertexLayout_;
    std::unordered_map<GraphicsPipelineHandle, std::unique_ptr<PipelineEntry>> pipelines_;
    std::unordered_map<GraphicsBufferHandle, std::unique_ptr<VertexBufferEntry>> vertexBuffers_;
    std::unordered_map<GraphicsBufferHandle, std::unique_ptr<IndexBufferEntry>> indexBuffers_;
    ViewState viewState_{};
    MaterialXUniforms materialXUniforms_{};
    uint32_t viewportWidth_ = 0;
    uint32_t viewportHeight_ = 0;
    bool initialized_ = false;
    bgfx::ViewId viewId_ = 0;
    PlatformHandleInfo platformHandleInfo_{};
    bgfx::PlatformData platformData_{};
    bool loggedInitFailureDiagnostics_ = false;
};

}  // namespace sdl3cpp::services::impl

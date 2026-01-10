#pragma once

#include "services/interfaces/i_config_service.hpp"
#include "services/interfaces/i_graphics_backend.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_platform_service.hpp"
#include "services/interfaces/i_pipeline_compiler_service.hpp"
#include "services/interfaces/i_probe_service.hpp"
#include "../../../core/vertex.hpp"
#include <bgfx/bgfx.h>
#include <array>
#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

namespace sdl3cpp::services::impl {

class BgfxGraphicsBackend : public IGraphicsBackend {
public:
    BgfxGraphicsBackend(std::shared_ptr<IConfigService> configService,
                        std::shared_ptr<IPlatformService> platformService,
                        std::shared_ptr<ILogger> logger,
                        std::shared_ptr<IPipelineCompilerService> pipelineCompiler,
                        std::shared_ptr<IProbeService> probeService = nullptr);
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
    void ConfigureView(GraphicsDeviceHandle device,
                       uint16_t viewId,
                       const ViewClearConfig& clearConfig) override;
    void Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
              GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
              uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset,
              const std::array<float, 16>& modelMatrix) override;
    void RequestScreenshot(GraphicsDeviceHandle device,
                           const std::filesystem::path& outputPath) override;
    GraphicsDeviceHandle GetPhysicalDevice() const override;
    std::pair<uint32_t, uint32_t> GetSwapchainExtent() const override;
    uint32_t GetSwapchainFormat() const override;
    void* GetCurrentCommandBuffer() const override;
    void* GetGraphicsQueue() const override;

private:
    friend bgfx::TextureHandle LoadTextureFromFileForTest(const BgfxGraphicsBackend& backend,
                                                          const std::string& path,
                                                          uint64_t samplerFlags);
    friend bool CanAllocateTextureBudgetForTest(const BgfxGraphicsBackend& backend, size_t bytes);
    friend void AllocateTextureBudgetForTest(BgfxGraphicsBackend& backend, size_t bytes);
    friend void FreeTextureBudgetForTest(BgfxGraphicsBackend& backend, size_t bytes);
    friend void SetTextureBudgetMaxForTest(BgfxGraphicsBackend& backend, size_t maxBytes);
    friend size_t GetTextureBudgetMaxForTest(const BgfxGraphicsBackend& backend);
    friend size_t GetTextureBudgetUsedForTest(const BgfxGraphicsBackend& backend);

    // Texture memory budget tracker to prevent GPU memory exhaustion
    class TextureMemoryTracker {
    public:
        TextureMemoryTracker() = default;

        bool CanAllocate(size_t bytes) const {
            if (maxBytes_ == 0) {
                return true;
            }
            return (totalBytes_ + bytes) <= maxBytes_;
        }

        void Allocate(size_t bytes) {
            totalBytes_ += bytes;
        }

        void Free(size_t bytes) {
            if (bytes > totalBytes_) {
                totalBytes_ = 0;
            } else {
                totalBytes_ -= bytes;
            }
        }

        size_t GetUsedBytes() const { return totalBytes_; }
        size_t GetMaxBytes() const { return maxBytes_; }
        size_t GetAvailableBytes() const { return maxBytes_ == 0 ? 0 : maxBytes_ - totalBytes_; }

        void SetMaxBytes(size_t max) { maxBytes_ = max; }

    private:
        size_t totalBytes_ = 0;
        size_t maxBytes_ = 512 * 1024 * 1024;  // 512MB default limit
    };

    struct PipelineEntry {
        bgfx::ProgramHandle program = BGFX_INVALID_HANDLE;
        struct TextureBinding {
            bgfx::UniformHandle sampler = BGFX_INVALID_HANDLE;
            bgfx::TextureHandle texture = BGFX_INVALID_HANDLE;
            uint8_t stage = 0;
            std::string uniformName;
            std::string sourcePath;
            size_t memorySizeBytes = 0;  // Track memory used by this texture
        };
        std::vector<TextureBinding> textures;
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
    bgfx::TextureHandle LoadTextureFromFile(const std::string& path,
                                            uint64_t samplerFlags,
                                            size_t* outSizeBytes = nullptr) const;
    void InitializeUniforms();
    void DestroyUniforms();
    void ApplyMaterialXUniforms(const std::array<float, 16>& modelMatrix);
    void DestroyPipelines();
    void DestroyBuffers();
    bool HasProcessedFrame() const { return frameCount_ > 0; }
    bool ShouldEmitRuntimeProbe() const;
    void ReportRuntimeProbe(const std::string& code,
                            const std::string& message,
                            const std::string& details = "") const;

    std::shared_ptr<IConfigService> configService_;
    std::shared_ptr<IPlatformService> platformService_;
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IPipelineCompilerService> pipelineCompiler_;
    std::shared_ptr<IProbeService> probeService_;
    bgfx::VertexLayout vertexLayout_;
    std::unordered_map<GraphicsPipelineHandle, std::unique_ptr<PipelineEntry>> pipelines_;
    std::unordered_map<GraphicsBufferHandle, std::unique_ptr<VertexBufferEntry>> vertexBuffers_;
    std::unordered_map<GraphicsBufferHandle, std::unique_ptr<IndexBufferEntry>> indexBuffers_;
    ViewState viewState_{};
    MaterialXUniforms materialXUniforms_{};
    uint32_t viewportWidth_ = 0;
    uint32_t viewportHeight_ = 0;
    bool bgfxInitialized_ = false;
    bool initialized_ = false;
    uint32_t frameCount_ = 0;
    bgfx::ViewId viewId_ = 0;
    PlatformHandleInfo platformHandleInfo_{};
    bgfx::PlatformData platformData_{};
    bool loggedInitFailureDiagnostics_ = false;
    mutable TextureMemoryTracker textureMemoryTracker_{};
    uint32_t maxTextureDim_ = 0;
};

}  // namespace sdl3cpp::services::impl

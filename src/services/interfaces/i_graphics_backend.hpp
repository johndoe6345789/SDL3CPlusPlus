#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <array>
#include <utility>
#include "services/interfaces/graphics_types.hpp"

namespace sdl3cpp::services {

/**
 * @brief Opaque handle for graphics device.
 */
using GraphicsDeviceHandle = void*;

/**
 * @brief Opaque handle for graphics pipeline.
 */
using GraphicsPipelineHandle = void*;

/**
 * @brief Opaque handle for buffer.
 */
using GraphicsBufferHandle = void*;

/**
 * @brief Opaque handle for texture.
 */
using GraphicsTextureHandle = void*;

/**
 * @brief Graphics backend interface for abstracted rendering.
 *
 * Provides backend-agnostic methods for device management, pipelines, buffers, and rendering.
 * Implementations handle platform-specific details (SDL3 GPU, GXM, etc.).
 */
class IGraphicsBackend {
public:
    virtual ~IGraphicsBackend() = default;

    /**
     * @brief Initialize the graphics backend.
     *
     * @param window Native window handle (SDL_Window* for desktop, Vita-specific for Vita)
     * @param config Graphics configuration
     */
    virtual void Initialize(void* window, const GraphicsConfig& config) = 0;

    /**
     * @brief Shutdown the graphics backend.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Recreate the swapchain for a new window size.
     *
     * @param width New width in pixels
     * @param height New height in pixels
     */
    virtual void RecreateSwapchain(uint32_t width, uint32_t height) = 0;

    /**
     * @brief Wait for GPU operations to complete.
     */
    virtual void WaitIdle() = 0;

    /**
     * @brief Create a graphics device.
     *
     * @return Opaque device handle
     */
    virtual GraphicsDeviceHandle CreateDevice() = 0;

    /**
     * @brief Destroy a graphics device.
     *
     * @param device Device handle
     */
    virtual void DestroyDevice(GraphicsDeviceHandle device) = 0;

    /**
     * @brief Create a graphics pipeline.
     *
     * @param device Device handle
     * @param shaderKey Unique key for the shader pipeline
     * @param shaderPaths Paths to vertex and fragment shaders
     * @return Opaque pipeline handle
     */
    virtual GraphicsPipelineHandle CreatePipeline(GraphicsDeviceHandle device, const std::string& shaderKey, const ShaderPaths& shaderPaths) = 0;

    /**
     * @brief Destroy a graphics pipeline.
     *
     * @param device Device handle
     * @param pipeline Pipeline handle
     */
    virtual void DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) = 0;

    /**
     * @brief Create a vertex buffer.
     *
     * @param device Device handle
     * @param data Vertex data
     * @return Opaque buffer handle
     */
    virtual GraphicsBufferHandle CreateVertexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) = 0;

    /**
     * @brief Create an index buffer.
     *
     * @param device Device handle
     * @param data Index data
     * @return Opaque buffer handle
     */
    virtual GraphicsBufferHandle CreateIndexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) = 0;

    /**
     * @brief Destroy a buffer.
     *
     * @param device Device handle
     * @param buffer Buffer handle
     */
    virtual void DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) = 0;

    /**
     * @brief Begin a frame.
     *
     * @param device Device handle
     * @return true if successful
     */
    virtual bool BeginFrame(GraphicsDeviceHandle device) = 0;

    /**
     * @brief End a frame.
     *
     * @param device Device handle
     * @return true if successful
     */
    virtual bool EndFrame(GraphicsDeviceHandle device) = 0;

    /**
     * @brief Request a screenshot of the backbuffer.
     *
     * @param device Device handle
     * @param outputPath Output path for the screenshot
     */
    virtual void RequestScreenshot(GraphicsDeviceHandle device,
                                   const std::filesystem::path& outputPath) = 0;

    /**
     * @brief Set the view state for the current frame.
     *
     * @param viewState Per-frame view state
     */
    virtual void SetViewState(const ViewState& viewState) = 0;

    /**
     * @brief Redirect rendering to the off-screen framebuffer (for screenshot capture).
     *
     * This allows screenshots to be captured reliably on all platforms,
     * especially Metal which has issues reading from the presentation surface.
     */
    virtual void SetViewFrameBuffer() = 0;

    /**
     * @brief Draw with a pipeline.
     *
     * @param device Device handle
     * @param pipeline Pipeline handle
     * @param vertexBuffer Vertex buffer handle
     * @param indexBuffer Index buffer handle
     * @param indexOffset Starting index offset
     * @param indexCount Number of indices
     * @param vertexOffset Base vertex offset
     * @param modelMatrix Model transformation matrix
     */
    virtual void Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
                      GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
                      uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset,
                      const std::array<float, 16>& modelMatrix) = 0;

    /**
     * @brief Get the physical device handle.
     *
     * @return Opaque physical device handle
     */
    virtual GraphicsDeviceHandle GetPhysicalDevice() const = 0;

    /**
     * @brief Get the swapchain extent.
     *
     * @return Width and height in pixels
     */
    virtual std::pair<uint32_t, uint32_t> GetSwapchainExtent() const = 0;

    /**
     * @brief Get the swapchain image format.
     *
     * @return Format identifier
     */
    virtual uint32_t GetSwapchainFormat() const = 0;

    /**
     * @brief Get the current command buffer.
     *
     * @return Opaque command buffer handle
     */
    virtual void* GetCurrentCommandBuffer() const = 0;

    /**
     * @brief Get the graphics queue handle.
     *
     * @return Opaque queue handle
     */
    virtual void* GetGraphicsQueue() const = 0;
};

}  // namespace sdl3cpp::services

#pragma once

#include "core/vertex.hpp"
#include "services/interfaces/graphics_types.hpp"
#include "services/interfaces/i_graphics_backend.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

// Forward declare SDL type
struct SDL_Window;

namespace sdl3cpp::services {

/**
 * @brief Graphics service interface (backend-agnostic rendering).
 *
 * Abstracts all rendering operations using opaque handles.
 */
class IGraphicsService {
public:
    virtual ~IGraphicsService() = default;

    /**
     * @brief Initialize the graphics backend.
     *
     * @param window The SDL window to create a surface for
     * @param config Graphics configuration
     * @throws std::runtime_error if initialization fails
     */
    virtual void InitializeDevice(SDL_Window* window, const GraphicsConfig& config) = 0;

    /**
     * @brief Initialize the swapchain for presenting rendered images.
     *
     * Must be called after InitializeDevice().
     *
     * @throws std::runtime_error if swapchain creation fails
     */
    virtual void InitializeSwapchain() = 0;

    /**
     * @brief Recreate the swapchain (e.g., after window resize).
     *
     * @throws std::runtime_error if recreation fails
     */
    virtual void RecreateSwapchain() = 0;

    /**
     * @brief Shutdown and release all resources.
     */
    virtual void Shutdown() noexcept = 0;

    /**
     * @brief Load and compile shader programs.
     *
     * @param shaders Map of shader key to shader file paths
     * @throws std::runtime_error if shader compilation fails
     */
    virtual void LoadShaders(const std::unordered_map<std::string, ShaderPaths>& shaders) = 0;

    /**
     * @brief Upload vertex data to GPU buffer.
     *
     * @param vertices Vector of vertex data
     * @throws std::runtime_error if buffer creation fails
     */
    virtual void UploadVertexData(const std::vector<core::Vertex>& vertices) = 0;

    /**
     * @brief Upload index data to GPU buffer.
     *
     * @param indices Vector of index data
     * @throws std::runtime_error if buffer creation fails
     */
    virtual void UploadIndexData(const std::vector<uint16_t>& indices) = 0;

    /**
     * @brief Begin a new frame and acquire the next swapchain image.
     *
     * @return true if successful, false if swapchain needs recreation
     */
    virtual bool BeginFrame() = 0;

    /**
     * @brief Render the scene with the given render commands.
     *
     * @param commands List of render commands to execute
     * @param viewProj View-projection matrix
     */
    virtual void RenderScene(const std::vector<RenderCommand>& commands,
                            const ViewState& viewState) = 0;

    /**
     * @brief End the frame and present the rendered image.
     *
     * @return true if successful, false if swapchain needs recreation
     */
    virtual bool EndFrame() = 0;

    /**
     * @brief Request a screenshot of the backbuffer.
     *
     * @param outputPath Output path for the screenshot
     */
    virtual void RequestScreenshot(const std::filesystem::path& outputPath) = 0;

    /**
     * @brief Wait for all GPU operations to complete.
     *
     * Called before cleanup or when synchronization is needed.
     */
    virtual void WaitIdle() = 0;

    /**
     * @brief Get the graphics device handle.
     *
     * @return Opaque device handle
     */
    virtual GraphicsDeviceHandle GetDevice() const = 0;

    /**
     * @brief Get the physical device handle.
     *
     * @return Opaque physical device handle
     */
    virtual GraphicsDeviceHandle GetPhysicalDevice() const = 0;

    /**
     * @brief Get the current swapchain extent (framebuffer size).
     *
     * @return Width and height
     */
    virtual std::pair<uint32_t, uint32_t> GetSwapchainExtent() const = 0;

    /**
     * @brief Get the swapchain image format.
     *
     * @return Format identifier
     */
    virtual uint32_t GetSwapchainFormat() const = 0;

    /**
     * @brief Get the current command buffer handle.
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

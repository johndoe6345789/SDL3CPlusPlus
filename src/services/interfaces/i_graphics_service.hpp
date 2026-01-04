#pragma once

#include "../../core/vertex.hpp"
#include "graphics_types.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

// Forward declare SDL type
struct SDL_Window;

namespace sdl3cpp::services {

/**
 * @brief Graphics service configuration.
 */
struct GraphicsConfig {
    std::vector<const char*> deviceExtensions;
    VkFormat preferredFormat = VK_FORMAT_B8G8R8A8_SRGB;
    bool enableValidationLayers = false;
};

/**
 * @brief Graphics service interface (Vulkan rendering).
 *
 * Abstracts all Vulkan rendering operations including device management,
 * swapchain, pipelines, buffers, and rendering.
 *
 * This is the largest service, consolidating ~1,500 lines of Vulkan code
 * from the original Sdl3App class.
 */
class IGraphicsService {
public:
    virtual ~IGraphicsService() = default;

    /**
     * @brief Initialize the Vulkan instance, device, and queues.
     *
     * @param window The SDL window to create a surface for
     * @param config Graphics configuration
     * @throws std::runtime_error if device initialization fails
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
     * @throws std::runtime_error if swapchain recreation fails
     */
    virtual void RecreateSwapchain() = 0;

    /**
     * @brief Shutdown and release all Vulkan resources.
     */
    virtual void Shutdown() = 0;

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
                            const std::array<float, 16>& viewProj) = 0;

    /**
     * @brief End the frame and present the rendered image.
     *
     * @return true if successful, false if swapchain needs recreation
     */
    virtual bool EndFrame() = 0;

    /**
     * @brief Wait for all GPU operations to complete.
     *
     * Called before cleanup or when synchronization is needed.
     */
    virtual void WaitIdle() = 0;

    /**
     * @brief Get the Vulkan logical device.
     *
     * @return VkDevice handle (needed for GUI renderer, etc.)
     */
    virtual VkDevice GetDevice() const = 0;

    /**
     * @brief Get the Vulkan physical device.
     *
     * @return VkPhysicalDevice handle
     */
    virtual VkPhysicalDevice GetPhysicalDevice() const = 0;

    /**
     * @brief Get the current swapchain extent (framebuffer size).
     *
     * @return VkExtent2D with width and height
     */
    virtual VkExtent2D GetSwapchainExtent() const = 0;

    /**
     * @brief Get the swapchain image format.
     *
     * @return VkFormat of swapchain images
     */
    virtual VkFormat GetSwapchainFormat() const = 0;

    /**
     * @brief Get the current command buffer being recorded.
     *
     * @return VkCommandBuffer (needed for GUI rendering)
     */
    virtual VkCommandBuffer GetCurrentCommandBuffer() const = 0;

    /**
     * @brief Get the graphics queue.
     *
     * @return VkQueue handle
     */
    virtual VkQueue GetGraphicsQueue() const = 0;
};

}  // namespace sdl3cpp::services

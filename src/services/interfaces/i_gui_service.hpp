#pragma once

#include <cstdint>
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {

// Forward declare GUI types
struct GuiCommand;

/**
 * @brief GUI rendering service interface.
 *
 * Handles 2D GUI overlay rendering using Vulkan.
 * Delegates low-level draw work to the GUI renderer service.
 */
class IGuiService {
public:
    virtual ~IGuiService() = default;

    /**
     * @brief Initialize the GUI renderer.
     *
     * @param device Vulkan logical device
     * @param physicalDevice Vulkan physical device
     * @param format Swapchain image format
     * @param renderPass Vulkan render pass for GUI pipeline creation
     * @param resourcePath Path to GUI resources (fonts, SVG files, etc.)
     * @throws std::runtime_error if initialization fails
     */
    virtual void Initialize(VkDevice device,
                           VkPhysicalDevice physicalDevice,
                           VkFormat format,
                           VkRenderPass renderPass,
                           const std::filesystem::path& resourcePath) = 0;

    /**
     * @brief Prepare GUI commands for rendering.
     *
     * Processes GUI commands from Lua and prepares rendering data.
     *
     * @param commands Vector of GUI commands to render
     * @param width Viewport width in pixels
     * @param height Viewport height in pixels
     */
    virtual void PrepareFrame(const std::vector<GuiCommand>& commands,
                             uint32_t width,
                             uint32_t height) = 0;

    /**
     * @brief Render GUI overlay to swapchain image.
     *
     * Records rendering commands into the provided command buffer.
     *
     * @param commandBuffer Vulkan command buffer to record into
     * @param image Target swapchain image
     */
    virtual void RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) = 0;

    /**
     * @brief Handle window resize.
     *
     * Recreates internal resources for the new window size.
     *
     * @param width New width in pixels
     * @param height New height in pixels
     * @param format Swapchain image format
     */
    virtual void Resize(uint32_t width, uint32_t height, VkFormat format, VkRenderPass renderPass) = 0;

    /**
     * @brief Shutdown and release GPU resources.
     */
    virtual void Shutdown() = 0;
};

}  // namespace sdl3cpp::services

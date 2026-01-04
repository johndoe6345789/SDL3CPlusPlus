#pragma once

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {

/**
 * @brief Swapchain support details.
 */
struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

/**
 * @brief Swapchain service interface.
 *
 * Manages Vulkan swapchain creation, recreation, and presentation.
 * Small, focused service (~250 lines) for swapchain management.
 */
class ISwapchainService {
public:
    virtual ~ISwapchainService() = default;

    /**
     * @brief Create the swapchain.
     *
     * @param width Desired width in pixels
     * @param height Desired height in pixels
     * @throws std::runtime_error if swapchain creation fails
     */
    virtual void CreateSwapchain(uint32_t width, uint32_t height) = 0;

    /**
     * @brief Recreate the swapchain (e.g., after window resize).
     *
     * @param width New width in pixels
     * @param height New height in pixels
     * @throws std::runtime_error if recreation fails
     */
    virtual void RecreateSwapchain(uint32_t width, uint32_t height) = 0;

    /**
     * @brief Cleanup the swapchain.
     *
     * Called before recreation or shutdown.
     */
    virtual void CleanupSwapchain() = 0;

    /**
     * @brief Acquire the next swapchain image.
     *
     * @param semaphore Semaphore to signal when image is available
     * @param imageIndex Output parameter for the image index
     * @return VkResult (VK_SUCCESS, VK_ERROR_OUT_OF_DATE_KHR, etc.)
     */
    virtual VkResult AcquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex) = 0;

    /**
     * @brief Present the rendered image to the screen.
     *
     * @param waitSemaphores Semaphores to wait on before presenting
     * @param imageIndex Index of the image to present
     * @return VkResult (VK_SUCCESS, VK_ERROR_OUT_OF_DATE_KHR, etc.)
     */
    virtual VkResult Present(const std::vector<VkSemaphore>& waitSemaphores,
                            uint32_t imageIndex) = 0;

    /**
     * @brief Get the swapchain handle.
     *
     * @return VkSwapchainKHR handle
     */
    virtual VkSwapchainKHR GetSwapchain() const = 0;

    /**
     * @brief Get swapchain images.
     *
     * @return Vector of swapchain image handles
     */
    virtual const std::vector<VkImage>& GetSwapchainImages() const = 0;

    /**
     * @brief Get swapchain image views.
     *
     * @return Vector of image view handles
     */
    virtual const std::vector<VkImageView>& GetSwapchainImageViews() const = 0;

    /**
     * @brief Get swapchain framebuffers.
     *
     * @return Vector of framebuffer handles
     */
    virtual const std::vector<VkFramebuffer>& GetSwapchainFramebuffers() const = 0;

    /**
     * @brief Get the swapchain image format.
     *
     * @return VkFormat of swapchain images
     */
    virtual VkFormat GetSwapchainImageFormat() const = 0;

    /**
     * @brief Get the swapchain extent (size).
     *
     * @return VkExtent2D with width and height
     */
    virtual VkExtent2D GetSwapchainExtent() const = 0;

    /**
     * @brief Get the render pass.
     *
     * @return VkRenderPass handle
     */
    virtual VkRenderPass GetRenderPass() const = 0;
};

}  // namespace sdl3cpp::services

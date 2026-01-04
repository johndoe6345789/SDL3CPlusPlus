#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

// Forward declare SDL type
struct SDL_Window;

namespace sdl3cpp::services {

/**
 * @brief Queue family indices for Vulkan.
 */
struct QueueFamilyIndices {
    uint32_t graphicsFamily = UINT32_MAX;
    uint32_t presentFamily = UINT32_MAX;

    bool IsComplete() const {
        return graphicsFamily != UINT32_MAX && presentFamily != UINT32_MAX;
    }
};

/**
 * @brief Vulkan device service interface.
 *
 * Manages Vulkan instance, physical device, logical device, and queues.
 * Small, focused service (~300 lines) for device initialization.
 */
class IVulkanDeviceService {
public:
    virtual ~IVulkanDeviceService() = default;

    /**
     * @brief Initialize Vulkan instance and select physical device.
     *
     * @param window SDL window for surface creation
     * @param deviceExtensions Required device extensions
     * @param enableValidationLayers Whether to enable validation layers
     * @throws std::runtime_error if initialization fails
     */
    virtual void Initialize(SDL_Window* window,
                           const std::vector<const char*>& deviceExtensions,
                           bool enableValidationLayers = false) = 0;

    /**
     * @brief Create the logical device and retrieve queues.
     *
     * Must be called after Initialize().
     *
     * @throws std::runtime_error if device creation fails
     */
    virtual void CreateLogicalDevice() = 0;

    /**
     * @brief Shutdown and release all Vulkan resources.
     */
    virtual void Shutdown() = 0;

    /**
     * @brief Wait for all device operations to complete.
     */
    virtual void WaitIdle() = 0;

    /**
     * @brief Get the Vulkan instance.
     *
     * @return VkInstance handle
     */
    virtual VkInstance GetInstance() const = 0;

    /**
     * @brief Get the Vulkan surface.
     *
     * @return VkSurfaceKHR handle
     */
    virtual VkSurfaceKHR GetSurface() const = 0;

    /**
     * @brief Get the physical device.
     *
     * @return VkPhysicalDevice handle
     */
    virtual VkPhysicalDevice GetPhysicalDevice() const = 0;

    /**
     * @brief Get the logical device.
     *
     * @return VkDevice handle
     */
    virtual VkDevice GetDevice() const = 0;

    /**
     * @brief Get the graphics queue.
     *
     * @return VkQueue handle
     */
    virtual VkQueue GetGraphicsQueue() const = 0;

    /**
     * @brief Get the present queue.
     *
     * @return VkQueue handle
     */
    virtual VkQueue GetPresentQueue() const = 0;

    /**
     * @brief Get queue family indices.
     *
     * @return Queue family indices structure
     */
    virtual QueueFamilyIndices GetQueueFamilies() const = 0;

    /**
     * @brief Find memory type index for allocation.
     *
     * @param typeFilter Memory type bits
     * @param properties Required memory properties
     * @return Memory type index
     * @throws std::runtime_error if no suitable memory type found
     */
    virtual uint32_t FindMemoryType(uint32_t typeFilter,
                                    VkMemoryPropertyFlags properties) const = 0;
};

}  // namespace sdl3cpp::services

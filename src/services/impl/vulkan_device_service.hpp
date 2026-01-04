#pragma once

#include "../interfaces/i_vulkan_device_service.hpp"
#include "../../di/lifecycle.hpp"
#include <vector>
#include <optional>

namespace sdl3cpp::services::impl {

/**
 * @brief Vulkan device service implementation.
 *
 * Small, focused service (~300 lines) for Vulkan device initialization.
 * Handles instance, physical device, logical device, and queue management.
 */
class VulkanDeviceService : public IVulkanDeviceService,
                            public di::IShutdownable {
public:
    VulkanDeviceService() = default;
    ~VulkanDeviceService() override;

    // IVulkanDeviceService interface
    void Initialize(const std::vector<const char*>& deviceExtensions,
                   bool enableValidationLayers = false) override;
    void CreateSurface(SDL_Window* window) override;
    void CreateLogicalDevice() override;
    void Shutdown() noexcept override;
    void WaitIdle() override;

    VkInstance GetInstance() const override { return instance_; }
    VkSurfaceKHR GetSurface() const override { return surface_; }
    VkPhysicalDevice GetPhysicalDevice() const override { return physicalDevice_; }
    VkDevice GetDevice() const override { return device_; }
    VkQueue GetGraphicsQueue() const override { return graphicsQueue_; }
    VkQueue GetPresentQueue() const override { return presentQueue_; }
    QueueFamilyIndices GetQueueFamilies() const override;

    uint32_t FindMemoryType(uint32_t typeFilter,
                           VkMemoryPropertyFlags properties) const override;

private:
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;

    std::vector<const char*> deviceExtensions_;
    bool validationLayersEnabled_ = false;

    // Helper methods
    void CreateInstance(const std::vector<const char*>& requiredExtensions);
    void PickPhysicalDevice();
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
};

}  // namespace sdl3cpp::services::impl

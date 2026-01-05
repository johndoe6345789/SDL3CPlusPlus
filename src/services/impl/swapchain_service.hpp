#pragma once

#include "../interfaces/i_swapchain_service.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include "../../events/i_event_bus.hpp"
#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Swapchain service implementation.
 *
 * Small, focused service (~250 lines) for Vulkan swapchain management.
 * Handles swapchain creation, recreation, image views, render pass, and framebuffers.
 */
class SwapchainService : public ISwapchainService,
                         public di::IInitializable,
                         public di::IShutdownable {
public:
    explicit SwapchainService(std::shared_ptr<IVulkanDeviceService> deviceService,
                             std::shared_ptr<events::IEventBus> eventBus,
                             std::shared_ptr<ILogger> logger);
    ~SwapchainService() override;

    // ISwapchainService interface
    void CreateSwapchain(uint32_t width, uint32_t height) override;
    void RecreateSwapchain(uint32_t width, uint32_t height) override;
    void CleanupSwapchain() override;

    VkResult AcquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex) override;
    VkResult Present(const std::vector<VkSemaphore>& waitSemaphores,
                     uint32_t imageIndex) override;

    VkSwapchainKHR GetSwapchain() const override {
        logger_->Trace("SwapchainService", "GetSwapchain");
        return swapchain_;
    }
    const std::vector<VkImage>& GetSwapchainImages() const override {
        logger_->Trace("SwapchainService", "GetSwapchainImages");
        return images_;
    }
    const std::vector<VkImageView>& GetSwapchainImageViews() const override {
        logger_->Trace("SwapchainService", "GetSwapchainImageViews");
        return imageViews_;
    }
    const std::vector<VkFramebuffer>& GetSwapchainFramebuffers() const override {
        logger_->Trace("SwapchainService", "GetSwapchainFramebuffers");
        return framebuffers_;
    }
    VkFormat GetSwapchainImageFormat() const override {
        logger_->Trace("SwapchainService", "GetSwapchainImageFormat");
        return imageFormat_;
    }
    VkExtent2D GetSwapchainExtent() const override {
        logger_->Trace("SwapchainService", "GetSwapchainExtent");
        return extent_;
    }
    VkRenderPass GetRenderPass() const override {
        logger_->Trace("SwapchainService", "GetRenderPass");
        return renderPass_;
    }

    // IInitializable interface
    void Initialize() override;

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    std::shared_ptr<IVulkanDeviceService> deviceService_;
    std::shared_ptr<events::IEventBus> eventBus_;
    std::shared_ptr<ILogger> logger_;

    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    std::vector<VkImage> images_;
    std::vector<VkImageView> imageViews_;
    VkFormat imageFormat_ = VK_FORMAT_UNDEFINED;
    VkExtent2D extent_{};
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers_;

    // Depth buffer resources
    VkFormat depthFormat_ = VK_FORMAT_UNDEFINED;
    std::vector<VkImage> depthImages_;
    std::vector<VkDeviceMemory> depthImageMemories_;
    std::vector<VkImageView> depthImageViews_;

    uint32_t currentWidth_ = 0;
    uint32_t currentHeight_ = 0;

    // Helper methods
    struct SwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities{};
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);
    VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, uint32_t width, uint32_t height);

    void CreateImageViews();
    void CreateDepthResources();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CleanupSwapchainInternal();

    VkFormat FindDepthFormat();
    bool HasStencilComponent(VkFormat format);

    void OnWindowResized(const events::Event& event);
};

}  // namespace sdl3cpp::services::impl

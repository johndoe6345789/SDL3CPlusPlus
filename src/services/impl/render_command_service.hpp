#pragma once

#include "../interfaces/i_render_command_service.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../interfaces/i_swapchain_service.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Render command service implementation.
 *
 * Small, focused service (~200 lines) for rendering orchestration.
 * Handles command buffer recording and frame synchronization.
 */
class RenderCommandService : public IRenderCommandService,
                              public di::IShutdownable {
public:
    explicit RenderCommandService(std::shared_ptr<IVulkanDeviceService> deviceService,
                                  std::shared_ptr<ISwapchainService> swapchainService);
    ~RenderCommandService() override;

    // IRenderCommandService interface
    void Cleanup() override;

    bool BeginFrame(uint32_t& imageIndex) override;
    void RecordCommands(uint32_t imageIndex,
                       const std::vector<RenderCommand>& commands,
                       const std::array<float, 16>& viewProj) override;
    bool EndFrame(uint32_t imageIndex) override;

    VkCommandBuffer GetCurrentCommandBuffer() const override;
    uint32_t GetCurrentFrameIndex() const override { return currentFrame_; }
    uint32_t GetMaxFramesInFlight() const override { return maxFramesInFlight_; }

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    std::shared_ptr<IVulkanDeviceService> deviceService_;
    std::shared_ptr<ISwapchainService> swapchainService_;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Synchronization primitives
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;

    uint32_t currentFrame_ = 0;
    uint32_t maxFramesInFlight_ = 1;  // Single frame in flight for simplicity

    // Helper methods
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CleanupCommandResources();
    void CleanupSyncObjects();
};

}  // namespace sdl3cpp::services::impl

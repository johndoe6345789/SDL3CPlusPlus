#pragma once

#include "../interfaces/i_render_command_service.hpp"
#include "../interfaces/i_buffer_service.hpp"
#include "../interfaces/i_pipeline_service.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../interfaces/i_swapchain_service.hpp"
#include "../interfaces/i_config_service.hpp"
#include "../interfaces/i_gui_renderer_service.hpp"
#include "../impl/json_config_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>
#include <unordered_map>
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
                                  std::shared_ptr<ISwapchainService> swapchainService,
                                  std::shared_ptr<IPipelineService> pipelineService,
                                  std::shared_ptr<IBufferService> bufferService,
                                  std::shared_ptr<IGuiRendererService> guiRendererService,
                                  std::shared_ptr<JsonConfigService> configService,
                                  std::shared_ptr<ILogger> logger);
    ~RenderCommandService() override;

    // IRenderCommandService interface
    void Cleanup() override;

    bool BeginFrame(uint32_t& imageIndex) override;
    void RecordCommands(uint32_t imageIndex,
                       const std::vector<RenderCommand>& commands,
                       const std::array<float, 16>& viewProj) override;
    void RecordRenderGraph(uint32_t imageIndex,
                           const RenderGraphDefinition& graph,
                           const std::vector<RenderCommand>& commands,
                           const std::array<float, 16>& viewProj) override;
    bool EndFrame(uint32_t imageIndex) override;

    VkCommandBuffer GetCurrentCommandBuffer() const override;
    uint32_t GetCurrentFrameIndex() const override {
        logger_->Trace("RenderCommandService", "GetCurrentFrameIndex");
        return currentFrame_;
    }
    uint32_t GetMaxFramesInFlight() const override {
        logger_->Trace("RenderCommandService", "GetMaxFramesInFlight");
        return maxFramesInFlight_;
    }
    void OnSwapchainRecreated() override;

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    struct RenderGraphImage {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkExtent2D extent{};
        VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    std::shared_ptr<IVulkanDeviceService> deviceService_;
    std::shared_ptr<ISwapchainService> swapchainService_;
    std::shared_ptr<IPipelineService> pipelineService_;
    std::shared_ptr<IBufferService> bufferService_;
    std::shared_ptr<IGuiRendererService> guiRendererService_;
    std::shared_ptr<JsonConfigService> configService_;
    std::shared_ptr<ILogger> logger_;

    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;

    // Synchronization primitives
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    VkFence inFlightFence_ = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool_ = VK_NULL_HANDLE;
    VkDescriptorSet defaultDescriptorSet_ = VK_NULL_HANDLE;
    VkDescriptorSet postProcessDescriptorSet_ = VK_NULL_HANDLE;
    VkSampler sampler_ = VK_NULL_HANDLE;
    VkImage dummyImage_ = VK_NULL_HANDLE;
    VkDeviceMemory dummyImageMemory_ = VK_NULL_HANDLE;
    VkImageView dummyImageView_ = VK_NULL_HANDLE;
    VkImageLayout dummyImageLayout_ = VK_IMAGE_LAYOUT_UNDEFINED;

    VkRenderPass offscreenRenderPass_ = VK_NULL_HANDLE;
    std::unordered_map<std::string, RenderGraphImage> renderGraphTargets_;
    std::unordered_map<std::string, RenderGraphImage> renderGraphDepth_;
    VkExtent2D renderGraphExtent_{};
    size_t renderGraphResourceCount_ = 0;
    size_t renderGraphPassCount_ = 0;

    uint32_t currentFrame_ = 0;
    uint32_t maxFramesInFlight_ = 1;  // Single frame in flight for simplicity

    // Helper methods
    void CreateCommandPool();
    void CreateCommandBuffers();
    void CreateSyncObjects();
    void CleanupCommandResources();
    void CleanupSyncObjects();

    void EnsureDescriptorResources();
    void CleanupDescriptorResources();
    void CreateDescriptorPool();
    void CreateSampler();
    void CreateDummyImage();
    void AllocateDescriptorSets();
    void UpdateDescriptorSet(VkDescriptorSet set, VkImageView view);
    void EnsureDummyImageLayout(VkCommandBuffer commandBuffer);

    void EnsureRenderGraphResources(const RenderGraphDefinition& graph);
    void CleanupRenderGraphResources();
    void RegisterRenderGraphShaders(const RenderGraphDefinition& graph);
    VkFormat ResolveColorFormat(const std::string& format) const;
    VkFormat ResolveDepthFormat(const std::string& format) const;
    VkExtent2D ResolveExtent(const RenderGraphResource& resource) const;
    RenderGraphImage* FindRenderTarget(const std::string& name);
    RenderGraphImage* FindDepthTarget(const std::string& name);
    void CreateRenderGraphImage(RenderGraphImage& image, VkFormat format, VkExtent2D extent,
                                VkImageUsageFlags usage, VkImageAspectFlags aspectMask);
    void TransitionImageLayout(VkCommandBuffer commandBuffer, RenderGraphImage& image,
                               VkImageLayout newLayout, VkImageAspectFlags aspectMask);
    bool IsScenePass(const RenderGraphPass& pass) const;
    bool IsPassEnabled(const RenderGraphPass& pass) const;
    std::string ResolvePassOutput(const RenderGraphPass& pass) const;
    std::string ResolvePassInput(const RenderGraphPass& pass) const;
    core::PushConstants BuildFullscreenConstants(const RenderGraphPass& pass) const;
};

}  // namespace sdl3cpp::services::impl

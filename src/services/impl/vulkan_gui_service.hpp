#pragma once

#include "../interfaces/i_gui_service.hpp"
#include "../interfaces/i_gui_renderer_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Vulkan GUI service implementation.
 *
 * Small wrapper service (~60 lines) around the GUI renderer service.
 * Provides 2D GUI overlay rendering for SVG, text, and shapes.
 */
class VulkanGuiService : public IGuiService,
                         public di::IShutdownable {
public:
    VulkanGuiService(std::shared_ptr<ILogger> logger,
                     std::shared_ptr<IGuiRendererService> rendererService);
    ~VulkanGuiService() override;

    // IGuiService interface
    void Initialize(VkDevice device,
                   VkPhysicalDevice physicalDevice,
                   VkFormat format,
                   VkRenderPass renderPass,
                   const std::filesystem::path& resourcePath) override;

    void PrepareFrame(const std::vector<GuiCommand>& commands,
                     uint32_t width,
                     uint32_t height) override;

    void RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) override;

    void Resize(uint32_t width, uint32_t height, VkFormat format, VkRenderPass renderPass) override;

    void Shutdown() noexcept override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IGuiRendererService> rendererService_;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl

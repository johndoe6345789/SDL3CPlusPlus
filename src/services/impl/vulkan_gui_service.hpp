#pragma once

#include "../interfaces/i_gui_service.hpp"
#include "../../gui/gui_renderer.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Vulkan GUI service implementation.
 *
 * Small wrapper service (~60 lines) around GuiRenderer.
 * Provides 2D GUI overlay rendering for SVG, text, and shapes.
 */
class VulkanGuiService : public IGuiService,
                         public di::IShutdownable {
public:
    VulkanGuiService() = default;
    ~VulkanGuiService() override;

    // IGuiService interface
    void Initialize(VkDevice device,
                   VkPhysicalDevice physicalDevice,
                   VkFormat format,
                   const std::filesystem::path& resourcePath) override;

    void PrepareFrame(const std::vector<GuiCommand>& commands,
                     uint32_t width,
                     uint32_t height) override;

    void RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) override;

    void Resize(uint32_t width, uint32_t height, VkFormat format) override;

    void Shutdown() noexcept override;

private:
    std::unique_ptr<gui::GuiRenderer> renderer_;
    bool initialized_ = false;
};

}  // namespace sdl3cpp::services::impl

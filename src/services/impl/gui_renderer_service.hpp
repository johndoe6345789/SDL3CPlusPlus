#pragma once

#include "../interfaces/i_buffer_service.hpp"
#include "../interfaces/i_gui_renderer_service.hpp"
#include "../interfaces/i_logger.hpp"
#include "../../di/lifecycle.hpp"
#include "../../gui/gui_renderer.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class GuiRendererService : public IGuiRendererService, public di::IShutdownable {
public:
    GuiRendererService(std::shared_ptr<ILogger> logger,
                       std::shared_ptr<IBufferService> bufferService);
    ~GuiRendererService() override = default;

    void Initialize(VkDevice device,
                    VkPhysicalDevice physicalDevice,
                    VkFormat format,
                    const std::filesystem::path& resourcePath) override;

    void PrepareFrame(const std::vector<script::GuiCommand>& commands,
                      uint32_t width,
                      uint32_t height) override;

    void RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) override;

    void Resize(uint32_t width, uint32_t height, VkFormat format) override;

    void Shutdown() noexcept override;

    bool IsReady() const override;

private:
    std::shared_ptr<ILogger> logger_;
    std::shared_ptr<IBufferService> bufferService_;
    std::unique_ptr<sdl3cpp::gui::GuiRenderer> renderer_;
};

}  // namespace sdl3cpp::services::impl

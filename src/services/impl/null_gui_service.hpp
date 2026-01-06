#pragma once

#include "../interfaces/i_gui_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

class NullGuiService : public IGuiService {
public:
    explicit NullGuiService(std::shared_ptr<ILogger> logger);

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
    void Shutdown() override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

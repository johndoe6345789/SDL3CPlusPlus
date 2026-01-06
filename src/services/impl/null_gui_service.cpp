#include "null_gui_service.hpp"

#include <string>

namespace sdl3cpp::services::impl {

NullGuiService::NullGuiService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("NullGuiService", "NullGuiService");
    }
}

void NullGuiService::Initialize(VkDevice device,
                                VkPhysicalDevice physicalDevice,
                                VkFormat format,
                                VkRenderPass renderPass,
                                const std::filesystem::path& resourcePath) {
    if (logger_) {
        logger_->Trace("NullGuiService", "Initialize");
    }
}

void NullGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                  uint32_t width,
                                  uint32_t height) {
    if (logger_) {
        logger_->Trace("NullGuiService", "PrepareFrame",
                       "commands.size=" + std::to_string(commands.size()));
    }
}

void NullGuiService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    if (logger_) {
        logger_->Trace("NullGuiService", "RenderToSwapchain");
    }
}

void NullGuiService::Resize(uint32_t width, uint32_t height, VkFormat format, VkRenderPass renderPass) {
    if (logger_) {
        logger_->Trace("NullGuiService", "Resize",
                       "width=" + std::to_string(width) +
                       ", height=" + std::to_string(height));
    }
}

void NullGuiService::Shutdown() {
    if (logger_) {
        logger_->Trace("NullGuiService", "Shutdown");
    }
}

}  // namespace sdl3cpp::services::impl

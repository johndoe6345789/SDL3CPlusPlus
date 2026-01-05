#include "gui_renderer_service.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

GuiRendererService::GuiRendererService(std::shared_ptr<ILogger> logger,
                                       std::shared_ptr<IBufferService> bufferService)
    : logger_(std::move(logger)),
      bufferService_(std::move(bufferService)) {
    if (logger_) {
        logger_->Trace("GuiRendererService", "GuiRendererService",
                       "bufferService=" + std::string(bufferService_ ? "set" : "null"));
    }
}

void GuiRendererService::Initialize(VkDevice device,
                                    VkPhysicalDevice physicalDevice,
                                    VkFormat format,
                                    VkRenderPass renderPass,
                                    const std::filesystem::path& resourcePath) {
    if (logger_) {
        logger_->Trace("GuiRendererService", "Initialize",
                       "deviceIsNull=" + std::string(device == VK_NULL_HANDLE ? "true" : "false") +
                       ", physicalDeviceIsNull=" + std::string(physicalDevice == VK_NULL_HANDLE ? "true" : "false") +
                       ", format=" + std::to_string(static_cast<uint32_t>(format)) +
                       ", renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false") +
                       ", resourcePath=" + resourcePath.string());
    }
    renderer_ = std::make_unique<GuiRenderer>(
        device, physicalDevice, format, renderPass, resourcePath, bufferService_, logger_);
}

void GuiRendererService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                      uint32_t width,
                                      uint32_t height) {
    if (logger_) {
        logger_->Trace("GuiRendererService", "PrepareFrame",
                       "commands.size=" + std::to_string(commands.size()) +
                       ", width=" + std::to_string(width) +
                       ", height=" + std::to_string(height));
    }
    if (!renderer_) {
        throw std::runtime_error("GuiRenderer service not initialized");
    }
    renderer_->Prepare(commands, width, height);
}

void GuiRendererService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    if (logger_) {
        logger_->Trace("GuiRendererService", "RenderToSwapchain",
                       "commandBufferIsNull=" + std::string(commandBuffer == VK_NULL_HANDLE ? "true" : "false") +
                       ", imageIsNull=" + std::string(image == VK_NULL_HANDLE ? "true" : "false"));
    }
    if (!renderer_) {
        throw std::runtime_error("GuiRenderer service not initialized");
    }
    // Note: image parameter is ignored - GUI renders into active render pass, not a specific image
    renderer_->RenderToSwapchain(commandBuffer, VK_NULL_HANDLE);
}

void GuiRendererService::Resize(uint32_t width, uint32_t height, VkFormat format, VkRenderPass renderPass) {
    if (logger_) {
        logger_->Trace("GuiRendererService", "Resize",
                       "width=" + std::to_string(width) +
                       ", height=" + std::to_string(height) +
                       ", format=" + std::to_string(static_cast<uint32_t>(format)) +
                       ", renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false"));
    }
    if (!renderer_) {
        return;
    }
    renderer_->Resize(width, height, format, renderPass);
}

void GuiRendererService::Shutdown() noexcept {
    if (logger_) {
        logger_->Trace("GuiRendererService", "Shutdown");
    }
    renderer_.reset();
}

bool GuiRendererService::IsReady() const {
    if (logger_) {
        logger_->Trace("GuiRendererService", "IsReady");
    }
    return renderer_ && renderer_->IsReady();
}

}  // namespace sdl3cpp::services::impl

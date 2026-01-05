#include "vulkan_gui_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

VulkanGuiService::VulkanGuiService(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<IGuiRendererService> rendererService)
    : logger_(std::move(logger)),
      rendererService_(std::move(rendererService)) {
    if (logger_) {
        logger_->Trace("VulkanGuiService", "VulkanGuiService",
                       "rendererService=" + std::string(rendererService_ ? "set" : "null"));
    }
}

VulkanGuiService::~VulkanGuiService() {
    if (logger_) {
        logger_->Trace("VulkanGuiService", "~VulkanGuiService");
    }
    if (initialized_) {
        Shutdown();
    }
}

void VulkanGuiService::Initialize(VkDevice device,
                                  VkPhysicalDevice physicalDevice,
                                  VkFormat format,
                                  VkRenderPass renderPass,
                                  const std::filesystem::path& resourcePath) {
    logger_->Trace("VulkanGuiService", "Initialize",
                   "deviceIsNull=" + std::string(device == VK_NULL_HANDLE ? "true" : "false") +
                   ", physicalDeviceIsNull=" + std::string(physicalDevice == VK_NULL_HANDLE ? "true" : "false") +
                   ", format=" + std::to_string(static_cast<uint32_t>(format)) +
                   ", renderPassIsNull=" + std::string(renderPass == VK_NULL_HANDLE ? "true" : "false") +
                   ", resourcePath=" + resourcePath.string());

    if (initialized_) {
        return;
    }

    if (!rendererService_) {
        throw std::runtime_error("GUI renderer service not available");
    }
    rendererService_->Initialize(device, physicalDevice, format, renderPass, resourcePath);
    initialized_ = true;

    logger_->Info("GUI service initialized");
}

void VulkanGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                   uint32_t width,
                                   uint32_t height) {
    logger_->Trace("VulkanGuiService", "PrepareFrame",
                   "commands.size=" + std::to_string(commands.size()) +
                   ", width=" + std::to_string(width) +
                   ", height=" + std::to_string(height));

    if (!rendererService_) {
        throw std::runtime_error("GUI renderer service not available");
    }
    rendererService_->PrepareFrame(commands, width, height);
}

void VulkanGuiService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    logger_->Trace("VulkanGuiService", "RenderToSwapchain",
                   "commandBufferIsNull=" + std::string(commandBuffer == VK_NULL_HANDLE ? "true" : "false") +
                   ", imageIsNull=" + std::string(image == VK_NULL_HANDLE ? "true" : "false"));

    if (!rendererService_) {
        throw std::runtime_error("GUI renderer service not available");
    }

    rendererService_->RenderToSwapchain(commandBuffer, image);
}

void VulkanGuiService::Resize(uint32_t width, uint32_t height, VkFormat format) {
    logger_->Trace("VulkanGuiService", "Resize",
                   "width=" + std::to_string(width) +
                   ", height=" + std::to_string(height) +
                   ", format=" + std::to_string(static_cast<uint32_t>(format)));

    if (rendererService_) {
        rendererService_->Resize(width, height, format);
    }
}

void VulkanGuiService::Shutdown() noexcept {
    logger_->Trace("VulkanGuiService", "Shutdown");

    if (!initialized_) {
        return;
    }

    if (rendererService_) {
        rendererService_->Shutdown();
    }
    initialized_ = false;

    logger_->Info("GUI service shutdown");
}

}  // namespace sdl3cpp::services::impl

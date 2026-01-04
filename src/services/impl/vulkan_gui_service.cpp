#include "vulkan_gui_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

VulkanGuiService::VulkanGuiService(std::shared_ptr<ILogger> logger,
                                   std::shared_ptr<IGuiRendererService> rendererService)
    : logger_(std::move(logger)),
      rendererService_(std::move(rendererService)) {
}

VulkanGuiService::~VulkanGuiService() {
    if (initialized_) {
        Shutdown();
    }
}

void VulkanGuiService::Initialize(VkDevice device,
                                  VkPhysicalDevice physicalDevice,
                                  VkFormat format,
                                  const std::filesystem::path& resourcePath) {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        return;
    }

    if (!rendererService_) {
        throw std::runtime_error("GUI renderer service not available");
    }
    rendererService_->Initialize(device, physicalDevice, format, resourcePath);
    initialized_ = true;

    logger_->Info("GUI service initialized");
}

void VulkanGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                   uint32_t width,
                                   uint32_t height) {
    logger_->TraceFunction(__func__);

    if (!rendererService_) {
        throw std::runtime_error("GUI renderer service not available");
    }
    rendererService_->PrepareFrame(commands, width, height);
}

void VulkanGuiService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    logger_->TraceFunction(__func__);

    if (!rendererService_) {
        throw std::runtime_error("GUI renderer service not available");
    }

    rendererService_->RenderToSwapchain(commandBuffer, image);
}

void VulkanGuiService::Resize(uint32_t width, uint32_t height, VkFormat format) {
    logger_->TraceFunction(__func__);

    if (rendererService_) {
        rendererService_->Resize(width, height, format);
    }
}

void VulkanGuiService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

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

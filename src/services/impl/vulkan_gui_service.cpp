#include "vulkan_gui_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

VulkanGuiService::VulkanGuiService(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {
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

    renderer_ = std::make_unique<gui::GuiRenderer>(device, physicalDevice, format, resourcePath);
    initialized_ = true;

    logger_->Info("GUI service initialized");
}

void VulkanGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                   uint32_t width,
                                   uint32_t height) {
    logger_->TraceFunction(__func__);

    if (!renderer_) {
        throw std::runtime_error("GUI service not initialized");
    }

    // GuiRenderer doesn't have a PrepareFrame method in the current implementation
    // Commands would be processed during RenderToSwapchain
}

void VulkanGuiService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    logger_->TraceFunction(__func__);

    if (!renderer_) {
        throw std::runtime_error("GUI service not initialized");
    }

    renderer_->BlitToSwapchain(commandBuffer, image);
}

void VulkanGuiService::Resize(uint32_t width, uint32_t height, VkFormat format) {
    logger_->TraceFunction(__func__);

    if (!renderer_) {
        return;
    }

    renderer_->Resize(width, height, format);
}

void VulkanGuiService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    renderer_.reset();
    initialized_ = false;

    logger_->Info("GUI service shutdown");
}

}  // namespace sdl3cpp::services::impl

#include "vulkan_gui_service.hpp"
#include "../../logging/logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

VulkanGuiService::~VulkanGuiService() {
    if (initialized_) {
        Shutdown();
    }
}

void VulkanGuiService::Initialize(VkDevice device,
                                  VkPhysicalDevice physicalDevice,
                                  VkFormat format,
                                  const std::filesystem::path& resourcePath) {
    logging::TraceGuard trace;

    if (initialized_) {
        return;
    }

    renderer_ = std::make_unique<gui::GuiRenderer>(device, physicalDevice, format, resourcePath);
    initialized_ = true;

    logging::Logger::GetInstance().Info("GUI service initialized");
}

void VulkanGuiService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                   uint32_t width,
                                   uint32_t height) {
    logging::TraceGuard trace;

    if (!renderer_) {
        throw std::runtime_error("GUI service not initialized");
    }

    // GuiRenderer doesn't have a PrepareFrame method in the current implementation
    // Commands would be processed during RenderToSwapchain
}

void VulkanGuiService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    logging::TraceGuard trace;

    if (!renderer_) {
        throw std::runtime_error("GUI service not initialized");
    }

    renderer_->BlitToSwapchain(commandBuffer, image);
}

void VulkanGuiService::Resize(uint32_t width, uint32_t height, VkFormat format) {
    logging::TraceGuard trace;

    if (!renderer_) {
        return;
    }

    renderer_->Resize(width, height, format);
}

void VulkanGuiService::Shutdown() noexcept {
    logging::TraceGuard trace;

    if (!initialized_) {
        return;
    }

    renderer_.reset();
    initialized_ = false;

    logging::Logger::GetInstance().Info("GUI service shutdown");
}

}  // namespace sdl3cpp::services::impl

#include "gui_renderer_service.hpp"

#include <stdexcept>
#include <utility>

namespace sdl3cpp::services::impl {

GuiRendererService::GuiRendererService(std::shared_ptr<ILogger> logger,
                                       std::shared_ptr<IBufferService> bufferService)
    : logger_(std::move(logger)),
      bufferService_(std::move(bufferService)) {
}

void GuiRendererService::Initialize(VkDevice device,
                                    VkPhysicalDevice physicalDevice,
                                    VkFormat format,
                                    const std::filesystem::path& resourcePath) {
    if (logger_) {
        logger_->TraceFunction(__func__);
    }
    renderer_ = std::make_unique<sdl3cpp::gui::GuiRenderer>(
        device, physicalDevice, format, resourcePath, bufferService_);
}

void GuiRendererService::PrepareFrame(const std::vector<GuiCommand>& commands,
                                      uint32_t width,
                                      uint32_t height) {
    if (logger_) {
        logger_->TraceFunction(__func__);
    }
    if (!renderer_) {
        throw std::runtime_error("GuiRenderer service not initialized");
    }
    renderer_->Prepare(commands, width, height);
}

void GuiRendererService::RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) {
    if (logger_) {
        logger_->TraceFunction(__func__);
    }
    if (!renderer_) {
        throw std::runtime_error("GuiRenderer service not initialized");
    }
    renderer_->BlitToSwapchain(commandBuffer, image);
}

void GuiRendererService::Resize(uint32_t width, uint32_t height, VkFormat format) {
    if (logger_) {
        logger_->TraceFunction(__func__);
    }
    if (!renderer_) {
        return;
    }
    renderer_->Resize(width, height, format);
}

void GuiRendererService::Shutdown() noexcept {
    if (logger_) {
        logger_->TraceFunction(__func__);
    }
    renderer_.reset();
}

bool GuiRendererService::IsReady() const {
    return renderer_ && renderer_->IsReady();
}

}  // namespace sdl3cpp::services::impl

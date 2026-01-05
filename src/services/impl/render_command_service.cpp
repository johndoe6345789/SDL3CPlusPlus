#include "render_command_service.hpp"
#include "../../core/vertex.hpp"
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

RenderCommandService::RenderCommandService(std::shared_ptr<IVulkanDeviceService> deviceService,
                                           std::shared_ptr<ISwapchainService> swapchainService,
                                           std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)),
      swapchainService_(std::move(swapchainService)),
      logger_(logger) {
    if (logger_) {
        logger_->Trace("RenderCommandService", "RenderCommandService",
                       "deviceService=" + std::string(deviceService_ ? "set" : "null") +
                       ", swapchainService=" + std::string(swapchainService_ ? "set" : "null"));
    }
}

RenderCommandService::~RenderCommandService() {
    if (logger_) {
        logger_->Trace("RenderCommandService", "~RenderCommandService");
    }
    if (commandPool_ != VK_NULL_HANDLE || imageAvailableSemaphore_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void RenderCommandService::Cleanup() {
    logger_->Trace("RenderCommandService", "Cleanup");
    CleanupCommandResources();
    CleanupSyncObjects();
}

void RenderCommandService::Shutdown() noexcept {
    logger_->Trace("RenderCommandService", "Shutdown");
    Cleanup();
}

bool RenderCommandService::BeginFrame(uint32_t& imageIndex) {
    logger_->Trace("RenderCommandService", "BeginFrame",
                   "imageIndex=" + std::to_string(imageIndex));

    // Lazy initialization
    if (commandPool_ == VK_NULL_HANDLE) {
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();
        logger_->Info("RenderCommandService initialized lazily");
    }

    auto device = deviceService_->GetDevice();

    // Wait for previous frame (with timeout)
    constexpr uint64_t kFenceTimeout = 5000000000ULL; // 5 seconds
    VkResult fenceResult = vkWaitForFences(device, 1, &inFlightFence_, VK_TRUE, kFenceTimeout);

    if (fenceResult == VK_TIMEOUT) {
        logger_->Error("Fence wait timeout: GPU appears to be hung");
        throw std::runtime_error("Fence wait timeout: GPU appears to be hung");
    } else if (fenceResult != VK_SUCCESS) {
        logger_->Error("Fence wait failed");
        throw std::runtime_error("Fence wait failed");
    }

    vkResetFences(device, 1, &inFlightFence_);

    // Acquire next image
    VkResult result = swapchainService_->AcquireNextImage(imageAvailableSemaphore_, imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;  // Need swapchain recreation
    } else if (result == VK_TIMEOUT) {
        logger_->Error("Image acquisition timeout");
        throw std::runtime_error("Image acquisition timeout: GPU appears to be hung");
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to acquire swap chain image");
    }

    return true;
}

void RenderCommandService::RecordCommands(uint32_t imageIndex,
                                         const std::vector<RenderCommand>& commands,
                                         const std::array<float, 16>& viewProj) {
    logger_->Trace("RenderCommandService", "RecordCommands",
                   "imageIndex=" + std::to_string(imageIndex) +
                   ", commands.size=" + std::to_string(commands.size()) +
                   ", viewProj.size=" + std::to_string(viewProj.size()));

    VkCommandBuffer commandBuffer = commandBuffers_[imageIndex];

    // Reset command buffer
    vkResetCommandBuffer(commandBuffer, 0);

    // Begin command buffer
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    // Begin render pass
    auto framebuffers = swapchainService_->GetSwapchainFramebuffers();
    auto extent = swapchainService_->GetSwapchainExtent();
    auto renderPass = swapchainService_->GetRenderPass();

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = extent;

    VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Record draw commands (placeholder - actual drawing would go here)
    // This would bind vertex buffers, index buffers, pipelines, and issue draw calls

    vkCmdEndRenderPass(commandBuffer);
    vkEndCommandBuffer(commandBuffer);
}

bool RenderCommandService::EndFrame(uint32_t imageIndex) {
    logger_->Trace("RenderCommandService", "EndFrame",
                   "imageIndex=" + std::to_string(imageIndex));

    auto device = deviceService_->GetDevice();
    auto graphicsQueue = deviceService_->GetGraphicsQueue();

    VkCommandBuffer commandBuffer = commandBuffers_[imageIndex];

    // Submit command buffer
    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit draw command buffer");
    }

    // Present
    std::vector<VkSemaphore> presentWaitSemaphores = {renderFinishedSemaphore_};
    VkResult result = swapchainService_->Present(presentWaitSemaphores, imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return false;  // Need swapchain recreation
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to present swap chain image");
    }

    return true;
}

VkCommandBuffer RenderCommandService::GetCurrentCommandBuffer() const {
    logger_->Trace("RenderCommandService", "GetCurrentCommandBuffer");
    if (commandBuffers_.empty()) {
        return VK_NULL_HANDLE;
    }
    return commandBuffers_[currentFrame_];
}

void RenderCommandService::CreateCommandPool() {
    logger_->Trace("RenderCommandService", "CreateCommandPool");

    auto device = deviceService_->GetDevice();
    auto queueFamilies = deviceService_->GetQueueFamilies();

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueFamilies.graphicsFamily;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create command pool");
    }
}

void RenderCommandService::CreateCommandBuffers() {
    logger_->Trace("RenderCommandService", "CreateCommandBuffers");

    auto device = deviceService_->GetDevice();
    auto framebuffers = swapchainService_->GetSwapchainFramebuffers();

    commandBuffers_.resize(framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool_;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers");
    }

    logger_->Debug("Created " + std::to_string(commandBuffers_.size()) + " command buffers");
}

void RenderCommandService::CreateSyncObjects() {
    logger_->Trace("RenderCommandService", "CreateSyncObjects");

    auto device = deviceService_->GetDevice();

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;  // Start signaled

    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create synchronization objects");
    }

    logger_->Debug("Created synchronization objects");
}

void RenderCommandService::CleanupCommandResources() {
    logger_->Trace("RenderCommandService", "CleanupCommandResources");

    auto device = deviceService_->GetDevice();

    if (!commandBuffers_.empty() && commandPool_ != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device, commandPool_,
                            static_cast<uint32_t>(commandBuffers_.size()),
                            commandBuffers_.data());
        commandBuffers_.clear();
    }

    if (commandPool_ != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, commandPool_, nullptr);
        commandPool_ = VK_NULL_HANDLE;
    }
}

void RenderCommandService::CleanupSyncObjects() {
    logger_->Trace("RenderCommandService", "CleanupSyncObjects");

    auto device = deviceService_->GetDevice();

    if (imageAvailableSemaphore_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, imageAvailableSemaphore_, nullptr);
        imageAvailableSemaphore_ = VK_NULL_HANDLE;
    }

    if (renderFinishedSemaphore_ != VK_NULL_HANDLE) {
        vkDestroySemaphore(device, renderFinishedSemaphore_, nullptr);
        renderFinishedSemaphore_ = VK_NULL_HANDLE;
    }

    if (inFlightFence_ != VK_NULL_HANDLE) {
        vkDestroyFence(device, inFlightFence_, nullptr);
        inFlightFence_ = VK_NULL_HANDLE;
    }
}

}  // namespace sdl3cpp::services::impl

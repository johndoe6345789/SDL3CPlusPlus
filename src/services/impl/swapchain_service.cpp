#include "swapchain_service.hpp"
#include <algorithm>
#include <stdexcept>

namespace sdl3cpp::services::impl {

SwapchainService::SwapchainService(std::shared_ptr<IVulkanDeviceService> deviceService,
                                   std::shared_ptr<events::EventBus> eventBus,
                                   std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)), eventBus_(std::move(eventBus)), logger_(logger) {
    // Subscribe to window resize events
    eventBus_->Subscribe(events::EventType::WindowResized,
                        [this](const events::Event& event) { OnWindowResized(event); });
}

SwapchainService::~SwapchainService() {
    if (swapchain_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void SwapchainService::Initialize() {
    logger_->TraceFunction(__func__);
    // Initialization happens in CreateSwapchain()
}

void SwapchainService::CreateSwapchain(uint32_t width, uint32_t height) {
    logger_->TraceFunction(__func__);

    currentWidth_ = width;
    currentHeight_ = height;

    auto physicalDevice = deviceService_->GetPhysicalDevice();
    auto surface = deviceService_->GetSurface();
    auto device = deviceService_->GetDevice();

    SwapchainSupportDetails support = QuerySwapchainSupport(physicalDevice, surface);

    // Validate swap chain support
    if (support.formats.empty()) {
        throw std::runtime_error("No surface formats available for swap chain.\n"
            "This may indicate GPU driver issues or incompatible surface.");
    }
    if (support.presentModes.empty()) {
        throw std::runtime_error("No present modes available for swap chain.\n"
            "This may indicate GPU driver issues or incompatible surface.");
    }

    logger_->Info("Creating swapchain with size: " + std::to_string(width) + "x" + std::to_string(height));

    if (width == 0 || height == 0) {
        logger_->Error("Invalid dimensions (" + std::to_string(width) + "x" + std::to_string(height) + ").");
        throw std::runtime_error("Invalid dimensions (" +
            std::to_string(width) + "x" + std::to_string(height) + ").\n" +
            "Window may be minimized or invalid.");
    }

    logger_->Debug("Surface capabilities - Min extent: " + std::to_string(support.capabilities.minImageExtent.width) + "x" + std::to_string(support.capabilities.minImageExtent.height) +
              ", Max extent: " + std::to_string(support.capabilities.maxImageExtent.width) + "x" + std::to_string(support.capabilities.maxImageExtent.height) +
              ", Min images: " + std::to_string(support.capabilities.minImageCount) +
              ", Max images: " + std::to_string(support.capabilities.maxImageCount));

    VkSurfaceFormatKHR surfaceFormat = ChooseSurfaceFormat(support.formats);
    VkPresentModeKHR presentMode = ChoosePresentMode(support.presentModes);
    VkExtent2D extent = ChooseExtent(support.capabilities, width, height);

    uint32_t imageCount = support.capabilities.minImageCount + 1;
    if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
        imageCount = support.capabilities.maxImageCount;
    }
    logger_->TraceVariable("imageCount", static_cast<int>(imageCount));

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices = deviceService_->GetQueueFamilies();
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = support.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain");
    }

    vkGetSwapchainImagesKHR(device, swapchain_, &imageCount, nullptr);
    images_.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain_, &imageCount, images_.data());

    imageFormat_ = surfaceFormat.format;
    extent_ = extent;

    CreateImageViews();
    CreateRenderPass();
    CreateFramebuffers();
}

void SwapchainService::RecreateSwapchain(uint32_t width, uint32_t height) {
    logger_->TraceFunction(__func__);

    logging::Logger::GetInstance().Info("Recreating swapchain: " + std::to_string(width) + "x" + std::to_string(height));

    deviceService_->WaitIdle();
    CleanupSwapchainInternal();
    CreateSwapchain(width, height);
}

VkResult SwapchainService::AcquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex) {
    auto device = deviceService_->GetDevice();
    return vkAcquireNextImageKHR(device, swapchain_, UINT64_MAX, semaphore,
                                 VK_NULL_HANDLE, &imageIndex);
}

VkResult SwapchainService::Present(const std::vector<VkSemaphore>& waitSemaphores,
                                   uint32_t imageIndex) {
    auto presentQueue = deviceService_->GetPresentQueue();

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    presentInfo.pWaitSemaphores = waitSemaphores.data();
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapchain_;
    presentInfo.pImageIndices = &imageIndex;

    return vkQueuePresentKHR(presentQueue, &presentInfo);
}

void SwapchainService::CreateImageViews() {
    logger_->TraceFunction(__func__);

    auto device = deviceService_->GetDevice();

    imageViews_.resize(images_.size());
    for (size_t i = 0; i < images_.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = images_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = imageFormat_;
        viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                               VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &imageViews_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views");
        }
    }
}

void SwapchainService::CreateRenderPass() {
    logger_->TraceFunction(__func__);

    auto device = deviceService_->GetDevice();

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = imageFormat_;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void SwapchainService::CreateFramebuffers() {
    logger_->TraceFunction(__func__);

    auto device = deviceService_->GetDevice();

    framebuffers_.resize(imageViews_.size());
    for (size_t i = 0; i < imageViews_.size(); ++i) {
        VkImageView attachments[] = {imageViews_[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = extent_.width;
        framebufferInfo.height = extent_.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void SwapchainService::CleanupSwapchainInternal() {
    logger_->TraceFunction(__func__);

    auto device = deviceService_->GetDevice();

    for (auto framebuffer : framebuffers_) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    framebuffers_.clear();

    if (renderPass_ != VK_NULL_HANDLE) {
        vkDestroyRenderPass(device, renderPass_, nullptr);
        renderPass_ = VK_NULL_HANDLE;
    }

    for (auto imageView : imageViews_) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    imageViews_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void SwapchainService::CleanupSwapchain() {
    CleanupSwapchainInternal();
}

void SwapchainService::Shutdown() noexcept {
    CleanupSwapchainInternal();
}

SwapchainService::SwapchainSupportDetails SwapchainService::QuerySwapchainSupport(
    VkPhysicalDevice device, VkSurfaceKHR surface) {
    logger_->TraceFunction(__func__);

    SwapchainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount,
                                                  details.presentModes.data());
    }

    return details;
}

VkSurfaceFormatKHR SwapchainService::ChooseSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    logger_->TraceFunction(__func__);

    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    return availableFormats[0];
}

VkPresentModeKHR SwapchainService::ChoosePresentMode(
    const std::vector<VkPresentModeKHR>& availablePresentModes) {
    logger_->TraceFunction(__func__);

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainService::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                          uint32_t width, uint32_t height) {
    return VkExtent2D{
        std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

void SwapchainService::OnWindowResized(const events::Event& event) {
    logger_->TraceFunction(__func__);
    logging::Logger::GetInstance().Info("Window resized event received, swapchain recreation needed");
}

}  // namespace sdl3cpp::services::impl

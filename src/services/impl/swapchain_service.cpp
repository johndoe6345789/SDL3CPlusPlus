#include "swapchain_service.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

SwapchainService::SwapchainService(std::shared_ptr<IVulkanDeviceService> deviceService,
                                   std::shared_ptr<events::IEventBus> eventBus,
                                   std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)), eventBus_(std::move(eventBus)), logger_(logger) {
    if (logger_) {
        logger_->Trace("SwapchainService", "SwapchainService",
                       "deviceService=" + std::string(deviceService_ ? "set" : "null") +
                       ", eventBus=" + std::string(eventBus_ ? "set" : "null"));
    }
    // Subscribe to window resize events
    eventBus_->Subscribe(events::EventType::WindowResized,
                        [this](const events::Event& event) { OnWindowResized(event); });
}

SwapchainService::~SwapchainService() {
    if (logger_) {
        logger_->Trace("SwapchainService", "~SwapchainService");
    }
    if (swapchain_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void SwapchainService::Initialize() {
    logger_->Trace("SwapchainService", "Initialize");
    // Initialization happens in CreateSwapchain()
}

void SwapchainService::CreateSwapchain(uint32_t width, uint32_t height) {
    logger_->Trace("SwapchainService", "CreateSwapchain",
                   "width=" + std::to_string(width) +
                   ", height=" + std::to_string(height));

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
    VkImageUsageFlags usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    if (support.capabilities.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_DST_BIT) {
        usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    createInfo.imageUsage = usage;

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
    CreateDepthResources();
    CreateRenderPass();
    CreateFramebuffers();
}

void SwapchainService::RecreateSwapchain(uint32_t width, uint32_t height) {
    logger_->Trace("SwapchainService", "RecreateSwapchain",
                   "width=" + std::to_string(width) +
                   ", height=" + std::to_string(height));

    logger_->Info("Recreating swapchain: " + std::to_string(width) + "x" + std::to_string(height));

    deviceService_->WaitIdle();
    CleanupSwapchainInternal();
    CreateSwapchain(width, height);
}

VkResult SwapchainService::AcquireNextImage(VkSemaphore semaphore, uint32_t& imageIndex) {
    logger_->Trace("SwapchainService", "AcquireNextImage",
                   "semaphoreIsNull=" + std::string(semaphore == VK_NULL_HANDLE ? "true" : "false") +
                   ", imageIndex=" + std::to_string(imageIndex));
    auto device = deviceService_->GetDevice();
    return vkAcquireNextImageKHR(device, swapchain_, UINT64_MAX, semaphore,
                                 VK_NULL_HANDLE, &imageIndex);
}

VkResult SwapchainService::Present(const std::vector<VkSemaphore>& waitSemaphores,
                                   uint32_t imageIndex) {
    logger_->Trace("SwapchainService", "Present",
                   "waitSemaphores.size=" + std::to_string(waitSemaphores.size()) +
                   ", imageIndex=" + std::to_string(imageIndex));
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
    logger_->Trace("SwapchainService", "CreateImageViews");

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
    logger_->Trace("SwapchainService", "CreateRenderPass");

    auto device = deviceService_->GetDevice();

    std::array<VkAttachmentDescription, 2> attachments{};

    // Color attachment
    attachments[0].format = imageFormat_;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    // Depth attachment
    attachments[1].format = depthFormat_;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                              VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                               VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create render pass");
    }
}

void SwapchainService::CreateFramebuffers() {
    logger_->Trace("SwapchainService", "CreateFramebuffers");

    auto device = deviceService_->GetDevice();

    framebuffers_.resize(imageViews_.size());
    for (size_t i = 0; i < imageViews_.size(); ++i) {
        std::array<VkImageView, 2> attachments = {imageViews_[i], depthImageViews_[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass_;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = extent_.width;
        framebufferInfo.height = extent_.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &framebuffers_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create framebuffer");
        }
    }
}

void SwapchainService::CleanupSwapchainInternal() {
    logger_->Trace("SwapchainService", "CleanupSwapchainInternal");

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

    for (auto imageView : depthImageViews_) {
        vkDestroyImageView(device, imageView, nullptr);
    }
    depthImageViews_.clear();

    for (size_t i = 0; i < depthImages_.size(); ++i) {
        vkDestroyImage(device, depthImages_[i], nullptr);
        vkFreeMemory(device, depthImageMemories_[i], nullptr);
    }
    depthImages_.clear();
    depthImageMemories_.clear();

    if (swapchain_ != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(device, swapchain_, nullptr);
        swapchain_ = VK_NULL_HANDLE;
    }
}

void SwapchainService::CleanupSwapchain() {
    logger_->Trace("SwapchainService", "CleanupSwapchain");
    CleanupSwapchainInternal();
}

void SwapchainService::Shutdown() noexcept {
    logger_->Trace("SwapchainService", "Shutdown");
    CleanupSwapchainInternal();
}

SwapchainService::SwapchainSupportDetails SwapchainService::QuerySwapchainSupport(
    VkPhysicalDevice device, VkSurfaceKHR surface) {
    logger_->Trace("SwapchainService", "QuerySwapchainSupport",
                   "deviceIsNull=" + std::string(device == VK_NULL_HANDLE ? "true" : "false") +
                   ", surfaceIsNull=" + std::string(surface == VK_NULL_HANDLE ? "true" : "false"));

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
    logger_->Trace("SwapchainService", "ChooseSurfaceFormat",
                   "availableFormats.size=" + std::to_string(availableFormats.size()));

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
    logger_->Trace("SwapchainService", "ChoosePresentMode",
                   "availablePresentModes.size=" + std::to_string(availablePresentModes.size()));

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D SwapchainService::ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities,
                                          uint32_t width, uint32_t height) {
    logger_->Trace("SwapchainService", "ChooseExtent",
                   "width=" + std::to_string(width) +
                   ", height=" + std::to_string(height) +
                   ", minWidth=" + std::to_string(capabilities.minImageExtent.width) +
                   ", minHeight=" + std::to_string(capabilities.minImageExtent.height) +
                   ", maxWidth=" + std::to_string(capabilities.maxImageExtent.width) +
                   ", maxHeight=" + std::to_string(capabilities.maxImageExtent.height));
    return VkExtent2D{
        std::clamp(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
        std::clamp(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
    };
}

VkFormat SwapchainService::FindDepthFormat() {
    logger_->Trace("SwapchainService", "FindDepthFormat");

    auto physicalDevice = deviceService_->GetPhysicalDevice();

    std::vector<VkFormat> candidates = {
        VK_FORMAT_D32_SFLOAT,
        VK_FORMAT_D32_SFLOAT_S8_UINT,
        VK_FORMAT_D24_UNORM_S8_UINT
    };

    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if ((props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            return format;
        }
    }

    throw std::runtime_error("Failed to find supported depth format");
}

bool SwapchainService::HasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void SwapchainService::CreateDepthResources() {
    logger_->Trace("SwapchainService", "CreateDepthResources");

    auto device = deviceService_->GetDevice();
    auto physicalDevice = deviceService_->GetPhysicalDevice();

    depthFormat_ = FindDepthFormat();

    VkExtent2D swapchainExtent = GetSwapchainExtent();
    depthImages_.resize(images_.size());
    depthImageMemories_.resize(images_.size());
    depthImageViews_.resize(images_.size());

    for (size_t i = 0; i < images_.size(); ++i) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = swapchainExtent.width;
        imageInfo.extent.height = swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = depthFormat_;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(device, &imageInfo, nullptr, &depthImages_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create depth image");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(device, depthImages_[i], &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = deviceService_->FindMemoryType(
            memRequirements.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device, &allocInfo, nullptr, &depthImageMemories_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate depth image memory");
        }

        vkBindImageMemory(device, depthImages_[i], depthImageMemories_[i], 0);

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthImages_[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = depthFormat_;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(depthFormat_)) {
            viewInfo.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &depthImageViews_[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create depth image view");
        }
    }

    logger_->Debug("Created depth resources for " + std::to_string(images_.size()) + " images");
}

void SwapchainService::OnWindowResized(const events::Event& event) {
    logger_->Trace("SwapchainService", "OnWindowResized",
                   "eventType=" + std::to_string(static_cast<int>(event.type)));
    logger_->Info("Window resized event received, swapchain recreation needed");
}

}  // namespace sdl3cpp::services::impl

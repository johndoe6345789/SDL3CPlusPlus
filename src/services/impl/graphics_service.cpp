#include "graphics_service.hpp"
#include "../interfaces/i_logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

GraphicsService::GraphicsService(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<IVulkanDeviceService> deviceService,
                                 std::shared_ptr<ISwapchainService> swapchainService,
                                 std::shared_ptr<IPipelineService> pipelineService,
                                 std::shared_ptr<IBufferService> bufferService,
                                 std::shared_ptr<IRenderCommandService> renderCommandService,
                                 std::shared_ptr<IWindowService> windowService)
    : logger_(std::move(logger)),
      deviceService_(deviceService),
      swapchainService_(swapchainService),
      pipelineService_(pipelineService),
      bufferService_(bufferService),
      renderCommandService_(renderCommandService),
      windowService_(windowService) {
    logger_->TraceFunction(__func__);

    if (!deviceService_ || !swapchainService_ || !pipelineService_ || !bufferService_ || !renderCommandService_ || !windowService_) {
        throw std::invalid_argument("All graphics services must be provided");
    }
}

GraphicsService::~GraphicsService() {
    logger_->TraceFunction(__func__);
    if (initialized_) {
        Shutdown();
    }
}

void GraphicsService::Initialize() {
    logger_->TraceFunction(__func__);

    if (initialized_) {
        throw std::runtime_error("Graphics service already initialized");
    }

    // Services are initialized individually by the registry
    initialized_ = true;
}

void GraphicsService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);

    // Services are shutdown individually by the registry
    initialized_ = false;
}

void GraphicsService::InitializeDevice(SDL_Window* window, const GraphicsConfig& config) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Device service handles device initialization
    deviceService_->Initialize(config.deviceExtensions, config.enableValidationLayers);
    deviceService_->CreateSurface(window);
    deviceService_->CreateLogicalDevice();
}

void GraphicsService::InitializeSwapchain() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Get window size and create swapchain
    auto [width, height] = windowService_->GetSize();
    swapchainService_->CreateSwapchain(width, height);
}

void GraphicsService::RecreateSwapchain() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Get current window size and recreate swapchain
    auto [width, height] = windowService_->GetSize();
    swapchainService_->RecreateSwapchain(width, height);
}

void GraphicsService::LoadShaders(const std::unordered_map<std::string, ShaderPaths>& shaders) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Convert shader paths map to the format expected by pipeline service
    for (const auto& [key, paths] : shaders) {
        pipelineService_->RegisterShader(key, paths);
    }
    pipelineService_->CompileAll(swapchainService_->GetRenderPass(), swapchainService_->GetSwapchainExtent());
}

void GraphicsService::UploadVertexData(const std::vector<core::Vertex>& vertices) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    bufferService_->UploadVertexData(vertices);
}

void GraphicsService::UploadIndexData(const std::vector<uint16_t>& indices) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    bufferService_->UploadIndexData(indices);
}

bool GraphicsService::BeginFrame() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return false;
    }

    return renderCommandService_->BeginFrame(currentImageIndex_);
}

void GraphicsService::RenderScene(const std::vector<RenderCommand>& commands,
                                 const std::array<float, 16>& viewProj) {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    renderCommandService_->RecordCommands(currentImageIndex_, commands, viewProj);
}

bool GraphicsService::EndFrame() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return false;
    }

    return renderCommandService_->EndFrame(currentImageIndex_);
}

void GraphicsService::WaitIdle() {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return;
    }

    deviceService_->WaitIdle();
}

VkDevice GraphicsService::GetDevice() const {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return deviceService_->GetDevice();
}

VkPhysicalDevice GraphicsService::GetPhysicalDevice() const {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return deviceService_->GetPhysicalDevice();
}

VkExtent2D GraphicsService::GetSwapchainExtent() const {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return {0, 0};
    }

    return swapchainService_->GetSwapchainExtent();
}

VkFormat GraphicsService::GetSwapchainFormat() const {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return VK_FORMAT_UNDEFINED;
    }

    return swapchainService_->GetSwapchainImageFormat();
}

VkCommandBuffer GraphicsService::GetCurrentCommandBuffer() const {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return renderCommandService_->GetCurrentCommandBuffer();
}

VkQueue GraphicsService::GetGraphicsQueue() const {
    logger_->TraceFunction(__func__);

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return deviceService_->GetGraphicsQueue();
}

}  // namespace sdl3cpp::services::impl
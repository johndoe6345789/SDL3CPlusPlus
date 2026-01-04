#include "graphics_service.hpp"
#include "../../logging/logger.hpp"
#include <stdexcept>

namespace sdl3cpp::services::impl {

GraphicsService::GraphicsService(std::shared_ptr<IVulkanDeviceService> deviceService,
                                 std::shared_ptr<ISwapchainService> swapchainService,
                                 std::shared_ptr<IPipelineService> pipelineService,
                                 std::shared_ptr<IBufferService> bufferService,
                                 std::shared_ptr<IRenderCommandService> renderCommandService)
    : deviceService_(deviceService),
      swapchainService_(swapchainService),
      pipelineService_(pipelineService),
      bufferService_(bufferService),
      renderCommandService_(renderCommandService) {
    logging::TraceGuard trace("GraphicsService::GraphicsService");

    if (!deviceService_ || !swapchainService_ || !pipelineService_ || !bufferService_ || !renderCommandService_) {
        throw std::invalid_argument("All graphics services must be provided");
    }
}

GraphicsService::~GraphicsService() {
    logging::TraceGuard trace("GraphicsService::~GraphicsService");
    if (initialized_) {
        Shutdown();
    }
}

void GraphicsService::Initialize() {
    logging::TraceGuard trace("GraphicsService::Initialize");

    if (initialized_) {
        throw std::runtime_error("Graphics service already initialized");
    }

    // Services are initialized individually by the registry
    initialized_ = true;
}

void GraphicsService::Shutdown() noexcept {
    logging::TraceGuard trace("GraphicsService::Shutdown");

    // Services are shutdown individually by the registry
    initialized_ = false;
}

void GraphicsService::InitializeDevice(SDL_Window* window, const GraphicsConfig& config) {
    logging::TraceGuard trace("GraphicsService::InitializeDevice");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Device service handles device initialization
    deviceService_->Initialize(config.deviceExtensions, config.enableValidationLayers);
    deviceService_->CreateSurface(window);
    deviceService_->CreateLogicalDevice();
}

void GraphicsService::InitializeSwapchain() {
    logging::TraceGuard trace("GraphicsService::InitializeSwapchain");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Swapchain service handles swapchain initialization
    swapchainService_->Initialize();
}

void GraphicsService::RecreateSwapchain() {
    logging::TraceGuard trace("GraphicsService::RecreateSwapchain");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    swapchainService_->RecreateSwapchain();
}

void GraphicsService::LoadShaders(const std::unordered_map<std::string, ShaderPaths>& shaders) {
    logging::TraceGuard trace("GraphicsService::LoadShaders");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Convert shader paths map to the format expected by pipeline service
    for (const auto& [key, paths] : shaders) {
        pipelineService_->RegisterShader(key, paths);
    }
    pipelineService_->CompileAll(swapchainService_->GetRenderPass(), swapchainService_->GetExtent());
}

void GraphicsService::UploadVertexData(const std::vector<core::Vertex>& vertices) {
    logging::TraceGuard trace("GraphicsService::UploadVertexData");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    bufferService_->UploadVertexData(vertices);
}

void GraphicsService::UploadIndexData(const std::vector<uint16_t>& indices) {
    logging::TraceGuard trace("GraphicsService::UploadIndexData");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    bufferService_->UploadIndexData(indices);
}

bool GraphicsService::BeginFrame() {
    logging::TraceGuard trace("GraphicsService::BeginFrame");

    if (!initialized_) {
        return false;
    }

    return renderCommandService_->BeginFrame();
}

void GraphicsService::RenderScene(const std::vector<RenderCommand>& commands,
                                 const std::array<float, 16>& viewProj) {
    logging::TraceGuard trace("GraphicsService::RenderScene");

    if (!initialized_) {
        return;
    }

    renderCommandService_->RecordCommands(commands, viewProj);
}

bool GraphicsService::EndFrame() {
    logging::TraceGuard trace("GraphicsService::EndFrame");

    if (!initialized_) {
        return false;
    }

    return renderCommandService_->EndFrame();
}

void GraphicsService::WaitIdle() {
    logging::TraceGuard trace("GraphicsService::WaitIdle");

    if (!initialized_) {
        return;
    }

    deviceService_->WaitIdle();
}

VkDevice GraphicsService::GetDevice() const {
    logging::TraceGuard trace("GraphicsService::GetDevice");

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return deviceService_->GetDevice();
}

VkPhysicalDevice GraphicsService::GetPhysicalDevice() const {
    logging::TraceGuard trace("GraphicsService::GetPhysicalDevice");

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return deviceService_->GetPhysicalDevice();
}

VkExtent2D GraphicsService::GetSwapchainExtent() const {
    logging::TraceGuard trace("GraphicsService::GetSwapchainExtent");

    if (!initialized_) {
        return {0, 0};
    }

    return swapchainService_->GetExtent();
}

uint32_t GraphicsService::GetSwapchainImageCount() const {
    logging::TraceGuard trace("GraphicsService::GetSwapchainImageCount");

    if (!initialized_) {
        return 0;
    }

    return swapchainService_->GetImageCount();
}

VkFormat GraphicsService::GetSwapchainFormat() const {
    logging::TraceGuard trace("GraphicsService::GetSwapchainFormat");

    if (!initialized_) {
        return VK_FORMAT_UNDEFINED;
    }

    return swapchainService_->GetFormat();
}

VkCommandBuffer GraphicsService::GetCurrentCommandBuffer() const {
    logging::TraceGuard trace("GraphicsService::GetCurrentCommandBuffer");

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return renderCommandService_->GetCurrentCommandBuffer();
}

VkQueue GraphicsService::GetGraphicsQueue() const {
    logging::TraceGuard trace("GraphicsService::GetGraphicsQueue");

    if (!initialized_) {
        return VK_NULL_HANDLE;
    }

    return deviceService_->GetGraphicsQueue();
}

}  // namespace sdl3cpp::services::impl
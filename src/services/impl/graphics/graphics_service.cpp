#include "graphics_service.hpp"
#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/graphics_types.hpp"
#include <stdexcept>
#include <cstring>

namespace sdl3cpp::services::impl {

GraphicsService::GraphicsService(std::shared_ptr<ILogger> logger,
                                 std::shared_ptr<IGraphicsBackend> backend,
                                 std::shared_ptr<IWindowService> windowService)
    : logger_(std::move(logger)),
      backend_(backend),
      windowService_(windowService) {
    logger_->Trace("GraphicsService", "GraphicsService",
                   "backend=" + std::string(backend_ ? "set" : "null") +
                   ", windowService=" + std::string(windowService_ ? "set" : "null"));

    if (!backend_ || !windowService_) {
        throw std::invalid_argument("Backend and window service must be provided");
    }
}

GraphicsService::~GraphicsService() {
    logger_->Trace("GraphicsService", "~GraphicsService");
    if (initialized_) {
        Shutdown();
    }
}

void GraphicsService::Initialize() {
    logger_->Trace("GraphicsService", "Initialize");

    if (initialized_) {
        throw std::runtime_error("Graphics service already initialized");
    }

    initialized_ = true;
}

void GraphicsService::Shutdown() noexcept {
    logger_->Trace("GraphicsService", "Shutdown");

    if (backend_) {
        backend_->Shutdown();
    }
    initialized_ = false;
}

void GraphicsService::InitializeDevice(SDL_Window* window, const GraphicsConfig& config) {
    logger_->Trace("GraphicsService", "InitializeDevice",
                   "windowIsNull=" + std::string(window ? "false" : "true") +
                   ", preferredFormat=" + std::to_string(config.preferredFormat));

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    backend_->Initialize(window, config);
    device_ = backend_->CreateDevice();
}

void GraphicsService::InitializeSwapchain() {
    logger_->Trace("GraphicsService", "InitializeSwapchain");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    // Swapchain is initialized in InitializeDevice via backend
}

void GraphicsService::RecreateSwapchain() {
    logger_->Trace("GraphicsService", "RecreateSwapchain");

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    auto size = windowService_ ? windowService_->GetSize() : std::pair<uint32_t, uint32_t>{0, 0};
    if (logger_) {
        logger_->Trace("GraphicsService", "RecreateSwapchain",
                       "windowSize=" + std::to_string(size.first) + "x" +
                       std::to_string(size.second));
    }
    if (size.first == 0 || size.second == 0) {
        logger_->Warn("GraphicsService::RecreateSwapchain: Skipping recreation for zero-size window");
        return;
    }

    backend_->RecreateSwapchain(size.first, size.second);
}

void GraphicsService::LoadShaders(const std::unordered_map<std::string, ShaderPaths>& shaders) {
    logger_->Trace("GraphicsService", "LoadShaders",
                   "shaders.size=" + std::to_string(shaders.size()));

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    for (const auto& [key, paths] : shaders) {
        auto pipeline = backend_->CreatePipeline(device_, key, paths);
        pipelines_[key] = pipeline;
    }
}

void GraphicsService::UploadVertexData(const std::vector<core::Vertex>& vertices) {
    logger_->Trace("GraphicsService", "UploadVertexData",
                   "vertices.size=" + std::to_string(vertices.size()));

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    if (vertexBuffer_ && backend_) {
        logger_->Trace("GraphicsService", "UploadVertexData",
                       "destroyingPreviousVertexBuffer=true");
        backend_->DestroyBuffer(device_, vertexBuffer_);
        vertexBuffer_ = nullptr;
    }

    // Convert vertices to bytes
    std::vector<uint8_t> data(sizeof(core::Vertex) * vertices.size());
    std::memcpy(data.data(), vertices.data(), data.size());
    vertexBuffer_ = backend_->CreateVertexBuffer(device_, data);
}

void GraphicsService::UploadIndexData(const std::vector<uint16_t>& indices) {
    logger_->Trace("GraphicsService", "UploadIndexData",
                   "indices.size=" + std::to_string(indices.size()));

    if (!initialized_) {
        throw std::runtime_error("Graphics service not initialized");
    }

    if (indexBuffer_ && backend_) {
        logger_->Trace("GraphicsService", "UploadIndexData",
                       "destroyingPreviousIndexBuffer=true");
        backend_->DestroyBuffer(device_, indexBuffer_);
        indexBuffer_ = nullptr;
    }

    // Convert indices to bytes
    std::vector<uint8_t> data(sizeof(uint16_t) * indices.size());
    std::memcpy(data.data(), indices.data(), data.size());
    indexBuffer_ = backend_->CreateIndexBuffer(device_, data);
}

bool GraphicsService::BeginFrame() {
    logger_->Trace("GraphicsService", "BeginFrame");

    if (!initialized_) {
        return false;
    }

    return backend_->BeginFrame(device_);
}

void GraphicsService::RenderScene(const std::vector<RenderCommand>& commands,
                                 const ViewState& viewState) {
    logger_->Trace("GraphicsService", "RenderScene",
                   "commands.size=" + std::to_string(commands.size()) +
                   ", viewProj.size=" + std::to_string(viewState.viewProj.size()));

    if (!initialized_) {
        return;
    }

    // Set the view-projection matrix for the frame
    backend_->SetViewState(viewState);

    if (commands.empty()) {
        return;
    }
    if (!vertexBuffer_ || !indexBuffer_) {
        logger_->Error("GraphicsService::RenderScene: Vertex/index buffers not uploaded");
        return;
    }

    for (size_t commandIndex = 0; commandIndex < commands.size(); ++commandIndex) {
        const auto& command = commands[commandIndex];
        if (command.shaderKeys.empty()) {
            logger_->Error("GraphicsService::RenderScene: Render command missing shader keys");
            continue;
        }
        if (logger_) {
            logger_->Trace("GraphicsService", "RenderScene",
                           "commandIndex=" + std::to_string(commandIndex) +
                           ", shaderKeyCount=" + std::to_string(command.shaderKeys.size()));
        }
        for (const auto& shaderKey : command.shaderKeys) {
            auto it = pipelines_.find(shaderKey);
            if (it == pipelines_.end()) {
                logger_->Error("GraphicsService::RenderScene: Missing pipeline for shaderKey=" + shaderKey);
                continue;
            }
            backend_->Draw(device_,
                           it->second,
                           vertexBuffer_,
                           indexBuffer_,
                           command.indexOffset,
                           command.indexCount,
                           command.vertexOffset,
                           command.modelMatrix);
        }
    }
}

void GraphicsService::ConfigureView(uint16_t viewId, const ViewClearConfig& clearConfig) {
    logger_->Trace("GraphicsService", "ConfigureView",
                   "viewId=" + std::to_string(viewId) +
                   ", clearEnabled=" + std::string(clearConfig.enabled ? "true" : "false"));

    if (!initialized_) {
        return;
    }

    backend_->ConfigureView(device_, viewId, clearConfig);
}

bool GraphicsService::EndFrame() {
    logger_->Trace("GraphicsService", "EndFrame");

    if (!initialized_) {
        return false;
    }

    return backend_->EndFrame(device_);
}

void GraphicsService::RequestScreenshot(const std::filesystem::path& outputPath) {
    logger_->Trace("GraphicsService", "RequestScreenshot",
                   "outputPath=" + outputPath.string());

    if (!initialized_) {
        return;
    }

    backend_->RequestScreenshot(device_, outputPath);
}

void GraphicsService::WaitIdle() {
    logger_->Trace("GraphicsService", "WaitIdle");

    if (!initialized_) {
        return;
    }

    backend_->WaitIdle();
}

GraphicsDeviceHandle GraphicsService::GetDevice() const {
    logger_->Trace("GraphicsService", "GetDevice");

    if (!initialized_) {
        return nullptr;
    }

    return device_;
}

GraphicsDeviceHandle GraphicsService::GetPhysicalDevice() const {
    logger_->Trace("GraphicsService", "GetPhysicalDevice");

    if (!initialized_) {
        return nullptr;
    }

    return backend_->GetPhysicalDevice();
}

std::pair<uint32_t, uint32_t> GraphicsService::GetSwapchainExtent() const {
    logger_->Trace("GraphicsService", "GetSwapchainExtent");

    if (!initialized_) {
        return {0, 0};
    }

    return backend_->GetSwapchainExtent();
}

uint32_t GraphicsService::GetSwapchainFormat() const {
    logger_->Trace("GraphicsService", "GetSwapchainFormat");

    if (!initialized_) {
        return 0;
    }

    return backend_->GetSwapchainFormat();
}

void* GraphicsService::GetCurrentCommandBuffer() const {
    logger_->Trace("GraphicsService", "GetCurrentCommandBuffer");

    if (!initialized_) {
        return nullptr;
    }

    return backend_->GetCurrentCommandBuffer();
}

void* GraphicsService::GetGraphicsQueue() const {
    logger_->Trace("GraphicsService", "GetGraphicsQueue");

    if (!initialized_) {
        return nullptr;
    }

    return backend_->GetGraphicsQueue();
}

}  // namespace sdl3cpp::services::impl

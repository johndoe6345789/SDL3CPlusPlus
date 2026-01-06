#include "vulkan_graphics_backend.hpp"

#include "../../core/vertex.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {

VulkanGraphicsBackend::VulkanGraphicsBackend(std::shared_ptr<IVulkanDeviceService> deviceService,
                                           std::shared_ptr<ISwapchainService> swapchainService,
                                           std::shared_ptr<IRenderCommandService> renderCommandService,
                                           std::shared_ptr<IPipelineService> pipelineService,
                                           std::shared_ptr<IBufferService> bufferService,
                                           std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)),
      swapchainService_(std::move(swapchainService)),
      renderCommandService_(std::move(renderCommandService)),
      pipelineService_(std::move(pipelineService)),
      bufferService_(std::move(bufferService)),
      logger_(std::move(logger)),
      window_(nullptr),
      initialized_(false),
      currentImageIndex_(0),
      frameCommands_(),
      currentViewProj_{} {
    logger_->Trace("VulkanGraphicsBackend", "VulkanGraphicsBackend",
                   "deviceService=" + std::string(deviceService_ ? "set" : "null") +
                   ", swapchainService=" + std::string(swapchainService_ ? "set" : "null") +
                   ", renderCommandService=" + std::string(renderCommandService_ ? "set" : "null") +
                   ", pipelineService=" + std::string(pipelineService_ ? "set" : "null") +
                   ", bufferService=" + std::string(bufferService_ ? "set" : "null"));
}

VulkanGraphicsBackend::~VulkanGraphicsBackend() {
    logger_->Trace("VulkanGraphicsBackend", "~VulkanGraphicsBackend");
    if (initialized_) {
        Shutdown();
    }
}

void VulkanGraphicsBackend::Initialize(void* window, const GraphicsConfig& config) {
    logger_->Trace("VulkanGraphicsBackend", "Initialize");

    if (initialized_) return;

    window_ = static_cast<SDL_Window*>(window);

    // Initialize Vulkan device
    std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    deviceService_->Initialize(deviceExtensions, config.enableValidationLayers);
    deviceService_->CreateSurface(window_);
    deviceService_->CreateLogicalDevice();

    // Get window size for swapchain
    int width, height;
    SDL_GetWindowSizeInPixels(window_, &width, &height);

    // Initialize swapchain
    swapchainService_->CreateSwapchain(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

    // Initialize render command service (this creates command pools/buffers)
    // Note: RenderCommandService initialization happens in its constructor

    initialized_ = true;
}

void VulkanGraphicsBackend::Shutdown() {
    logger_->Trace("VulkanGraphicsBackend", "Shutdown");

    if (!initialized_) return;

    // Cleanup in reverse order
    renderCommandService_->Cleanup();
    pipelineService_->Cleanup();
    bufferService_->Cleanup();
    swapchainService_->CleanupSwapchain();
    deviceService_->Shutdown();

    initialized_ = false;
}

void VulkanGraphicsBackend::RecreateSwapchain(uint32_t width, uint32_t height) {
    logger_->Trace("VulkanGraphicsBackend", "RecreateSwapchain",
                   "width=" + std::to_string(width) +
                   ", height=" + std::to_string(height));

    if (!initialized_) {
        return;
    }

    if (width == 0 || height == 0) {
        logger_->Warn("VulkanGraphicsBackend::RecreateSwapchain: Skipping swapchain recreation for zero size");
        return;
    }

    deviceService_->WaitIdle();
    swapchainService_->RecreateSwapchain(width, height);

    VkExtent2D extent = swapchainService_->GetSwapchainExtent();
    pipelineService_->RecreatePipelines(swapchainService_->GetRenderPass(), extent);
    renderCommandService_->OnSwapchainRecreated();
}

void VulkanGraphicsBackend::WaitIdle() {
    logger_->Trace("VulkanGraphicsBackend", "WaitIdle");
    deviceService_->WaitIdle();
}

GraphicsDeviceHandle VulkanGraphicsBackend::CreateDevice() {
    logger_->Trace("VulkanGraphicsBackend", "CreateDevice");
    // Device is already created in Initialize, just return a handle
    return reinterpret_cast<GraphicsDeviceHandle>(deviceService_->GetDevice());
}

void VulkanGraphicsBackend::DestroyDevice(GraphicsDeviceHandle device) {
    logger_->Trace("VulkanGraphicsBackend", "DestroyDevice");
    // Device cleanup happens in Shutdown
}

GraphicsPipelineHandle VulkanGraphicsBackend::CreatePipeline(GraphicsDeviceHandle device, const std::string& shaderKey, const ShaderPaths& shaderPaths) {
    logger_->Trace("VulkanGraphicsBackend", "CreatePipeline", "shaderKey=" + shaderKey);

    // Register shader with pipeline service
    pipelineService_->RegisterShader(shaderKey, shaderPaths);

    // Compile pipeline with render pass from swapchain service
    // Note: This assumes swapchain service has created the render pass
    VkExtent2D extent = swapchainService_->GetSwapchainExtent();
    if (logger_) {
        logger_->Trace("VulkanGraphicsBackend", "CreatePipeline",
                       "swapchainExtent=" + std::to_string(extent.width) + "x" +
                       std::to_string(extent.height));
    }

    pipelineService_->CompileAll(swapchainService_->GetRenderPass(), extent);

    // Return the pipeline handle
    VkPipeline pipeline = pipelineService_->GetPipeline(shaderKey);
    GraphicsPipelineHandle handle = reinterpret_cast<GraphicsPipelineHandle>(pipeline);
    
    // Store the mapping from handle to key
    pipelineToShaderKey_[handle] = shaderKey;
    
    return handle;
}

void VulkanGraphicsBackend::DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) {
    logger_->Trace("VulkanGraphicsBackend", "DestroyPipeline");
    // Pipeline cleanup happens in pipeline service Shutdown
}

GraphicsBufferHandle VulkanGraphicsBackend::CreateVertexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) {
    logger_->Trace("VulkanGraphicsBackend", "CreateVertexBuffer", "data.size=" + std::to_string(data.size()));

    if (data.empty()) {
        logger_->Error("VulkanGraphicsBackend::CreateVertexBuffer: No vertex data provided");
        return nullptr;
    }

    if (data.size() % sizeof(core::Vertex) != 0) {
        logger_->Error("VulkanGraphicsBackend::CreateVertexBuffer: Vertex data size is not aligned to Vertex");
        return nullptr;
    }

    const size_t vertexCount = data.size() / sizeof(core::Vertex);
    if (logger_) {
        logger_->Trace("VulkanGraphicsBackend", "CreateVertexBuffer",
                       "vertexCount=" + std::to_string(vertexCount));
    }

    std::vector<core::Vertex> vertices(vertexCount);
    std::memcpy(vertices.data(), data.data(), data.size());
    bufferService_->UploadVertexData(vertices);

    return reinterpret_cast<GraphicsBufferHandle>(bufferService_->GetVertexBuffer());
}

GraphicsBufferHandle VulkanGraphicsBackend::CreateIndexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) {
    logger_->Trace("VulkanGraphicsBackend", "CreateIndexBuffer", "data.size=" + std::to_string(data.size()));

    if (data.empty()) {
        logger_->Error("VulkanGraphicsBackend::CreateIndexBuffer: No index data provided");
        return nullptr;
    }

    if (data.size() % sizeof(uint16_t) != 0) {
        logger_->Error("VulkanGraphicsBackend::CreateIndexBuffer: Index data size is not aligned to uint16_t");
        return nullptr;
    }

    const size_t indexCount = data.size() / sizeof(uint16_t);
    if (logger_) {
        logger_->Trace("VulkanGraphicsBackend", "CreateIndexBuffer",
                       "indexCount=" + std::to_string(indexCount));
    }

    std::vector<uint16_t> indices(indexCount);
    std::memcpy(indices.data(), data.data(), data.size());
    bufferService_->UploadIndexData(indices);

    return reinterpret_cast<GraphicsBufferHandle>(bufferService_->GetIndexBuffer());
}

void VulkanGraphicsBackend::DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) {
    logger_->Trace("VulkanGraphicsBackend", "DestroyBuffer");
    // Buffer cleanup happens in buffer service Shutdown
}

bool VulkanGraphicsBackend::BeginFrame(GraphicsDeviceHandle device) {
    logger_->Trace("VulkanGraphicsBackend", "BeginFrame");

    frameCommands_.clear();
    return renderCommandService_->BeginFrame(currentImageIndex_);
}

bool VulkanGraphicsBackend::EndFrame(GraphicsDeviceHandle device) {
    logger_->Trace("VulkanGraphicsBackend", "EndFrame");

    // Record all accumulated commands
    if (renderGraphEnabled_) {
        renderCommandService_->RecordRenderGraph(currentImageIndex_, renderGraphDefinition_,
                                                 frameCommands_, currentViewProj_);
    } else {
        renderCommandService_->RecordCommands(currentImageIndex_, frameCommands_, currentViewProj_);
    }
    
    // End the frame
    return renderCommandService_->EndFrame(currentImageIndex_);
}

void VulkanGraphicsBackend::SetViewProjection(const std::array<float, 16>& viewProj) {
    logger_->Trace("VulkanGraphicsBackend", "SetViewProjection");
    currentViewProj_ = viewProj;
}

void VulkanGraphicsBackend::SetRenderGraphDefinition(const RenderGraphDefinition& definition) {
    logger_->Trace("VulkanGraphicsBackend", "SetRenderGraphDefinition",
                   "resources=" + std::to_string(definition.resources.size()) +
                   ", passes=" + std::to_string(definition.passes.size()));
    renderGraphDefinition_ = definition;
    renderGraphEnabled_ = !definition.passes.empty();
}

void VulkanGraphicsBackend::Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
                                 GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
                                 uint32_t indexCount, const std::array<float, 16>& modelMatrix) {
    logger_->Trace("VulkanGraphicsBackend", "Draw", "indexCount=" + std::to_string(indexCount));

    // Find the shader key for this pipeline
    auto it = pipelineToShaderKey_.find(pipeline);
    if (it == pipelineToShaderKey_.end()) {
        logger_->Error("VulkanGraphicsBackend::Draw: Pipeline not found in mapping");
        return;
    }

    // Create a render command
    RenderCommand command;
    command.indexOffset = 0;  // TODO: Calculate proper offset
    command.indexCount = indexCount;
    command.vertexOffset = 0;  // TODO: Calculate proper offset
    command.shaderKey = it->second;
    command.modelMatrix = modelMatrix;

    // Accumulate the command for later recording
    frameCommands_.push_back(command);
}

GraphicsDeviceHandle VulkanGraphicsBackend::GetPhysicalDevice() const {
    logger_->Trace("VulkanGraphicsBackend", "GetPhysicalDevice");
    return reinterpret_cast<GraphicsDeviceHandle>(deviceService_->GetPhysicalDevice());
}

std::pair<uint32_t, uint32_t> VulkanGraphicsBackend::GetSwapchainExtent() const {
    logger_->Trace("VulkanGraphicsBackend", "GetSwapchainExtent");
    VkExtent2D extent = swapchainService_->GetSwapchainExtent();
    return {extent.width, extent.height};
}

uint32_t VulkanGraphicsBackend::GetSwapchainFormat() const {
    logger_->Trace("VulkanGraphicsBackend", "GetSwapchainFormat");
    return static_cast<uint32_t>(swapchainService_->GetSwapchainImageFormat());
}

void* VulkanGraphicsBackend::GetCurrentCommandBuffer() const {
    logger_->Trace("VulkanGraphicsBackend", "GetCurrentCommandBuffer");
    return reinterpret_cast<void*>(renderCommandService_->GetCurrentCommandBuffer());
}

void* VulkanGraphicsBackend::GetGraphicsQueue() const {
    logger_->Trace("VulkanGraphicsBackend", "GetGraphicsQueue");
    return reinterpret_cast<void*>(deviceService_->GetGraphicsQueue());
}

}  // namespace sdl3cpp::services::impl

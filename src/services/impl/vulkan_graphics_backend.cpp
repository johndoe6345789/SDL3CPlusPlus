#include "vulkan_graphics_backend.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
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
    SDL_GetWindowSize(window_, &width, &height);

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
    VkExtent2D extent;
    // TODO: Get extent from swapchain service
    extent.width = 800;  // Temporary
    extent.height = 600; // Temporary

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

    // For now, we'll use the buffer service's existing vertex buffer functionality
    // This is a bit of a mismatch - the buffer service expects core::Vertex, but we get raw bytes
    // TODO: Extend buffer service to handle raw buffer creation or create a new method

    // Return a dummy handle for now
    return reinterpret_cast<GraphicsBufferHandle>(bufferService_->GetVertexBuffer());
}

GraphicsBufferHandle VulkanGraphicsBackend::CreateIndexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) {
    logger_->Trace("VulkanGraphicsBackend", "CreateIndexBuffer", "data.size=" + std::to_string(data.size()));

    // Similar issue as vertex buffer
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
    renderCommandService_->RecordCommands(currentImageIndex_, frameCommands_, currentViewProj_);
    
    // End the frame
    return renderCommandService_->EndFrame(currentImageIndex_);
}

void VulkanGraphicsBackend::SetViewProjection(const std::array<float, 16>& viewProj) {
    logger_->Trace("VulkanGraphicsBackend", "SetViewProjection");
    currentViewProj_ = viewProj;
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

}  // namespace sdl3cpp::services::impl
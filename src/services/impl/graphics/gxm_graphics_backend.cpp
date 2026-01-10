#include "gxm_graphics_backend.hpp"
#include "../../../core/vertex.hpp"
#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <psp2/io/fcntl.h>
#include <stdexcept>
#include <vector>
#include <array>
#include <cstring>
#include <iostream>
#include <fstream>

namespace sdl3cpp::services::impl {

// Vita display dimensions
static const unsigned int DISPLAY_WIDTH = 960;
static const unsigned int DISPLAY_HEIGHT = 544;
static const unsigned int DISPLAY_STRIDE_IN_PIXELS = 1024;
static const unsigned int DISPLAY_BUFFER_COUNT = 2;

// Memory sizes for GXM
static const unsigned int VDM_RING_BUFFER_SIZE = 128 * 1024;
static const unsigned int VERTEX_RING_BUFFER_SIZE = 2 * 1024 * 1024;
static const unsigned int FRAGMENT_RING_BUFFER_SIZE = 1024 * 1024;
static const unsigned int FRAGMENT_USSE_RING_BUFFER_SIZE = 64 * 1024;

GxmGraphicsBackend::GxmGraphicsBackend()
    : context_(nullptr), renderTarget_(nullptr), shaderPatcher_(nullptr),
      displayBufferData_{nullptr, nullptr}, displayBufferUid_{0, 0},
      displayBufferSync_{nullptr, nullptr}, backBufferIndex_(0), frontBufferIndex_(0),
      initialized_(false), vdmRingBuffer_(nullptr), vertexRingBuffer_(nullptr),
      fragmentRingBuffer_(nullptr), fragmentUsseRingBuffer_(nullptr),
      vdmRingBufferUid_(0), vertexRingBufferUid_(0), fragmentRingBufferUid_(0),
      fragmentUsseRingBufferUid_(0),
      displayWidth_(DISPLAY_WIDTH),
      displayHeight_(DISPLAY_HEIGHT) {
}

GxmGraphicsBackend::~GxmGraphicsBackend() {
    if (initialized_) {
        Shutdown();
    }
}

void GxmGraphicsBackend::Initialize(void* window, const GraphicsConfig& config) {
    std::cout << "GXM: Initializing GXM graphics backend" << std::endl;

    int err;
    (void)window;
    (void)config;

    // Initialize GXM
    SceGxmInitializeParams gxmInitParams;
    memset(&gxmInitParams, 0, sizeof(gxmInitParams));
    gxmInitParams.flags = 0;
    gxmInitParams.displayQueueMaxPendingCount = DISPLAY_BUFFER_COUNT - 1;
    gxmInitParams.displayQueueCallback = nullptr;
    gxmInitParams.displayQueueCallbackDataSize = 0;
    gxmInitParams.parameterBufferSize = SCE_GXM_DEFAULT_PARAMETER_BUFFER_SIZE;

    err = sceGxmInitialize(&gxmInitParams);
    if (err != SCE_OK) {
        throw std::runtime_error("Failed to initialize GXM: " + std::to_string(err));
    }

    // Allocate ring buffers
    vdmRingBufferUid_ = sceKernelAllocMemBlock("vdmRingBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, VDM_RING_BUFFER_SIZE, nullptr);
    if (vdmRingBufferUid_ < 0) {
        sceGxmTerminate();
        throw std::runtime_error("Failed to allocate VDM ring buffer");
    }
    err = sceKernelGetMemBlockBase(vdmRingBufferUid_, &vdmRingBuffer_);
    if (err != SCE_OK) {
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to get VDM ring buffer base");
    }

    vertexRingBufferUid_ = sceKernelAllocMemBlock("vertexRingBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, VERTEX_RING_BUFFER_SIZE, nullptr);
    if (vertexRingBufferUid_ < 0) {
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to allocate vertex ring buffer");
    }
    err = sceKernelGetMemBlockBase(vertexRingBufferUid_, &vertexRingBuffer_);
    if (err != SCE_OK) {
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to get vertex ring buffer base");
    }

    fragmentRingBufferUid_ = sceKernelAllocMemBlock("fragmentRingBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, FRAGMENT_RING_BUFFER_SIZE, nullptr);
    if (fragmentRingBufferUid_ < 0) {
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to allocate fragment ring buffer");
    }
    err = sceKernelGetMemBlockBase(fragmentRingBufferUid_, &fragmentRingBuffer_);
    if (err != SCE_OK) {
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to get fragment ring buffer base");
    }

    fragmentUsseRingBufferUid_ = sceKernelAllocMemBlock("fragmentUsseRingBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW_UNCACHE, FRAGMENT_USSE_RING_BUFFER_SIZE, nullptr);
    if (fragmentUsseRingBufferUid_ < 0) {
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to allocate fragment USSE ring buffer");
    }
    err = sceKernelGetMemBlockBase(fragmentUsseRingBufferUid_, &fragmentUsseRingBuffer_);
    if (err != SCE_OK) {
        sceKernelFreeMemBlock(fragmentUsseRingBufferUid_);
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to get fragment USSE ring buffer base");
    }

    // Create GXM context
    SceGxmContextParams contextParams;
    memset(&contextParams, 0, sizeof(contextParams));
    contextParams.hostMem = malloc(SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE);
    contextParams.hostMemSize = SCE_GXM_MINIMUM_CONTEXT_HOST_MEM_SIZE;
    contextParams.vdmRingBufferMem = vdmRingBuffer_;
    contextParams.vdmRingBufferMemSize = VDM_RING_BUFFER_SIZE;
    contextParams.vertexRingBufferMem = vertexRingBuffer_;
    contextParams.vertexRingBufferMemSize = VERTEX_RING_BUFFER_SIZE;
    contextParams.fragmentRingBufferMem = fragmentRingBuffer_;
    contextParams.fragmentRingBufferMemSize = FRAGMENT_RING_BUFFER_SIZE;
    contextParams.fragmentUsseRingBufferMem = fragmentUsseRingBuffer_;
    contextParams.fragmentUsseRingBufferMemSize = FRAGMENT_USSE_RING_BUFFER_SIZE;

    err = sceGxmCreateContext(&contextParams, &context_);
    if (err != SCE_OK) {
        free(contextParams.hostMem);
        sceKernelFreeMemBlock(fragmentUsseRingBufferUid_);
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to create GXM context: " + std::to_string(err));
    }

    // Create shader patcher
    SceGxmShaderPatcherParams shaderPatcherParams;
    memset(&shaderPatcherParams, 0, sizeof(shaderPatcherParams));
    shaderPatcherParams.userData = nullptr;
    shaderPatcherParams.hostAllocCallback = nullptr;
    shaderPatcherParams.hostFreeCallback = nullptr;
    shaderPatcherParams.bufferAllocCallback = nullptr;
    shaderPatcherParams.bufferFreeCallback = nullptr;
    shaderPatcherParams.bufferMem = nullptr;
    shaderPatcherParams.bufferMemSize = 0;
    shaderPatcherParams.vertexUsseAllocCallback = nullptr;
    shaderPatcherParams.vertexUsseFreeCallback = nullptr;
    shaderPatcherParams.vertexUsseMem = nullptr;
    shaderPatcherParams.vertexUsseMemSize = 0;
    shaderPatcherParams.vertexUsseOffset = 0;
    shaderPatcherParams.fragmentUsseAllocCallback = nullptr;
    shaderPatcherParams.fragmentUsseFreeCallback = nullptr;
    shaderPatcherParams.fragmentUsseMem = nullptr;
    shaderPatcherParams.fragmentUsseMemSize = 0;
    shaderPatcherParams.fragmentUsseOffset = 0;

    err = sceGxmShaderPatcherCreate(&shaderPatcherParams, &shaderPatcher_);
    if (err != SCE_OK) {
        sceGxmDestroyContext(context_);
        free(contextParams.hostMem);
        sceKernelFreeMemBlock(fragmentUsseRingBufferUid_);
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to create shader patcher: " + std::to_string(err));
    }

    // Create display buffers
    err = createDisplayBuffers();
    if (err != SCE_OK) {
        sceGxmShaderPatcherDestroy(shaderPatcher_);
        sceGxmDestroyContext(context_);
        free(contextParams.hostMem);
        sceKernelFreeMemBlock(fragmentUsseRingBufferUid_);
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to create display buffers: " + std::to_string(err));
    }

    // Create render target
    SceGxmRenderTargetParams renderTargetParams;
    memset(&renderTargetParams, 0, sizeof(renderTargetParams));
    renderTargetParams.width = DISPLAY_WIDTH;
    renderTargetParams.height = DISPLAY_HEIGHT;
    renderTargetParams.scenesPerFrame = 1;
    renderTargetParams.flags = 0;
    renderTargetParams.multisampleMode = SCE_GXM_MULTISAMPLE_NONE;

    err = sceGxmCreateRenderTarget(&renderTargetParams, &renderTarget_);
    if (err != SCE_OK) {
        destroyDisplayBuffers();
        sceGxmShaderPatcherDestroy(shaderPatcher_);
        sceGxmDestroyContext(context_);
        free(contextParams.hostMem);
        sceKernelFreeMemBlock(fragmentUsseRingBufferUid_);
        sceKernelFreeMemBlock(fragmentRingBufferUid_);
        sceKernelFreeMemBlock(vertexRingBufferUid_);
        sceKernelFreeMemBlock(vdmRingBufferUid_);
        sceGxmTerminate();
        throw std::runtime_error("Failed to create render target: " + std::to_string(err));
    }

    std::cout << "GXM: Initialization complete" << std::endl;
    initialized_ = true;
}

void GxmGraphicsBackend::Shutdown() {
    if (!initialized_) return;

    std::cout << "GXM: Shutting down GXM graphics backend" << std::endl;

    destroyShaderPrograms();

    if (renderTarget_) {
        sceGxmDestroyRenderTarget(renderTarget_);
        renderTarget_ = nullptr;
    }

    destroyDisplayBuffers();

    if (shaderPatcher_) {
        sceGxmShaderPatcherDestroy(shaderPatcher_);
        shaderPatcher_ = nullptr;
    }

    if (context_) {
        sceGxmDestroyContext(context_);
        context_ = nullptr;
    }

    sceKernelFreeMemBlock(fragmentUsseRingBufferUid_);
    sceKernelFreeMemBlock(fragmentRingBufferUid_);
    sceKernelFreeMemBlock(vertexRingBufferUid_);
    sceKernelFreeMemBlock(vdmRingBufferUid_);

    sceGxmTerminate();
    initialized_ = false;
}

void GxmGraphicsBackend::RecreateSwapchain(uint32_t width, uint32_t height) {
    std::cout << "GXM: Swapchain recreation not supported on Vita (" << width << "x" << height << ")" << std::endl;
}

void GxmGraphicsBackend::WaitIdle() {
    if (context_) {
        sceGxmFinish(context_);
    }
}

GraphicsDeviceHandle GxmGraphicsBackend::CreateDevice() {
    std::cout << "GXM: Creating device handle" << std::endl;
    return static_cast<GraphicsDeviceHandle>(context_);
}

void GxmGraphicsBackend::DestroyDevice(GraphicsDeviceHandle device) {
    std::cout << "GXM: Destroying device handle" << std::endl;
    // Context is destroyed in Shutdown
}

GraphicsPipelineHandle GxmGraphicsBackend::CreatePipeline(GraphicsDeviceHandle device, const std::string& shaderKey, const ShaderPaths& shaderPaths) {
    std::cout << "GXM: Creating pipeline for shader: " << shaderKey << std::endl;
    std::cout << "GXM: Vertex shader: " << shaderPaths.vertex << std::endl;
    std::cout << "GXM: Fragment shader: " << shaderPaths.fragment << std::endl;

    int err = createShaderPrograms(shaderKey, shaderPaths);
    if (err != SCE_OK) {
        std::cerr << "Failed to create shader programs for " << shaderKey << ": " << err << std::endl;
        return nullptr;
    }

    // Return a handle that combines vertex and fragment programs
    auto vertexIt = vertexPrograms_.find(shaderKey);
    auto fragmentIt = fragmentPrograms_.find(shaderKey);
    if (vertexIt == vertexPrograms_.end() || fragmentIt == fragmentPrograms_.end()) {
        return nullptr;
    }

    // Create a simple struct to hold both programs
    struct PipelineHandle {
        SceGxmVertexProgram* vertexProgram;
        SceGxmFragmentProgram* fragmentProgram;
    };

    PipelineHandle* handle = new PipelineHandle{vertexIt->second, fragmentIt->second};
    return static_cast<GraphicsPipelineHandle>(handle);
}

void GxmGraphicsBackend::DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) {
    std::cout << "GXM: Destroying pipeline" << std::endl;
    if (pipeline) {
        struct PipelineHandle {
            SceGxmVertexProgram* vertexProgram;
            SceGxmFragmentProgram* fragmentProgram;
        };
        PipelineHandle* handle = static_cast<PipelineHandle*>(pipeline);
        delete handle;
    }
}

GraphicsBufferHandle GxmGraphicsBackend::CreateVertexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) {
    std::cout << "GXM: Creating vertex buffer with " << data.size() << " bytes" << std::endl;

    // Allocate CDRAM for vertex buffer
    SceUID bufferUid = sceKernelAllocMemBlock("vertexBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, data.size(), nullptr);
    if (bufferUid < 0) {
        std::cerr << "Failed to allocate vertex buffer memory" << std::endl;
        return nullptr;
    }

    void* bufferData;
    int err = sceKernelGetMemBlockBase(bufferUid, &bufferData);
    if (err != SCE_OK) {
        sceKernelFreeMemBlock(bufferUid);
        std::cerr << "Failed to get vertex buffer base address" << std::endl;
        return nullptr;
    }

    // Copy data to buffer
    memcpy(bufferData, data.data(), data.size());

    // Store UID for cleanup
    struct BufferHandle {
        void* data;
        SceUID uid;
    };

    BufferHandle* handle = new BufferHandle{bufferData, bufferUid};
    return static_cast<GraphicsBufferHandle>(handle);
}

GraphicsBufferHandle GxmGraphicsBackend::CreateIndexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) {
    std::cout << "GXM: Creating index buffer with " << data.size() << " bytes" << std::endl;

    // Allocate CDRAM for index buffer
    SceUID bufferUid = sceKernelAllocMemBlock("indexBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW, data.size(), nullptr);
    if (bufferUid < 0) {
        std::cerr << "Failed to allocate index buffer memory" << std::endl;
        return nullptr;
    }

    void* bufferData;
    int err = sceKernelGetMemBlockBase(bufferUid, &bufferData);
    if (err != SCE_OK) {
        sceKernelFreeMemBlock(bufferUid);
        std::cerr << "Failed to get index buffer base address" << std::endl;
        return nullptr;
    }

    // Copy data to buffer
    memcpy(bufferData, data.data(), data.size());

    // Store UID for cleanup
    struct BufferHandle {
        void* data;
        SceUID uid;
    };

    BufferHandle* handle = new BufferHandle{bufferData, bufferUid};
    return static_cast<GraphicsBufferHandle>(handle);
}

void GxmGraphicsBackend::DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) {
    std::cout << "GXM: Destroying buffer" << std::endl;
    if (buffer) {
        struct BufferHandle {
            void* data;
            SceUID uid;
        };
        BufferHandle* handle = static_cast<BufferHandle*>(buffer);
        sceKernelFreeMemBlock(handle->uid);
        delete handle;
    }
}

bool GxmGraphicsBackend::BeginFrame(GraphicsDeviceHandle device) {
    std::cout << "GXM: Beginning frame" << std::endl;

    int err;

    // Set up color surface for current back buffer
    err = sceGxmColorSurfaceInit(&colorSurface_, SCE_GXM_COLOR_FORMAT_A8B8G8R8, SCE_GXM_COLOR_SURFACE_LINEAR,
                                SCE_GXM_COLOR_SURFACE_SCALE_NONE, SCE_GXM_OUTPUT_REGISTER_SIZE_32BIT,
                                DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_STRIDE_IN_PIXELS,
                                displayBufferData_[backBufferIndex_]);
    if (err != SCE_OK) {
        std::cerr << "Failed to initialize color surface: " << err << std::endl;
        return false;
    }

    // Set up depth/stencil surface (disabled for now)
    err = sceGxmDepthStencilSurfaceInitDisabled(&depthStencilSurface_);
    if (err != SCE_OK) {
        std::cerr << "Failed to initialize depth stencil surface: " << err << std::endl;
        return false;
    }

    // Begin scene
    err = sceGxmBeginScene(context_, 0, renderTarget_, nullptr, nullptr, displayBufferSync_[backBufferIndex_],
                          &colorSurface_, &depthStencilSurface_);
    if (err != SCE_OK) {
        std::cerr << "Failed to begin scene: " << err << std::endl;
        return false;
    }

    return true;
}

bool GxmGraphicsBackend::EndFrame(GraphicsDeviceHandle device) {
    std::cout << "GXM: Ending frame" << std::endl;

    // End scene
    int err = sceGxmEndScene(context_, nullptr, nullptr);
    if (err != SCE_OK) {
        std::cerr << "Failed to end scene: " << err << std::endl;
        return false;
    }

    // Present the back buffer
    SceDisplayFrameBuf frameBuf;
    memset(&frameBuf, 0, sizeof(frameBuf));
    frameBuf.size = sizeof(frameBuf);
    frameBuf.base = displayBufferData_[backBufferIndex_];
    frameBuf.pitch = DISPLAY_STRIDE_IN_PIXELS;
    frameBuf.pixelformat = SCE_DISPLAY_PIXELFORMAT_A8B8G8R8;
    frameBuf.width = DISPLAY_WIDTH;
    frameBuf.height = DISPLAY_HEIGHT;

    err = sceDisplaySetFrameBuf(&frameBuf, SCE_DISPLAY_SETBUF_NEXTFRAME);
    if (err != SCE_OK) {
        std::cerr << "Failed to set frame buffer: " << err << std::endl;
        return false;
    }

    // Swap buffers
    frontBufferIndex_ = backBufferIndex_;
    backBufferIndex_ = (backBufferIndex_ + 1) % DISPLAY_BUFFER_COUNT;

    // Wait for vblank
    sceDisplayWaitVblankStart();

    return true;
}

void GxmGraphicsBackend::RequestScreenshot(GraphicsDeviceHandle device,
                                           const std::filesystem::path& outputPath) {
    (void)device;
    (void)outputPath;
}

void GxmGraphicsBackend::SetViewState(const ViewState& viewState) {
    std::cout << "GXM: Setting view state" << std::endl;
    (void)viewState;
    // Matrix will be set when drawing with specific pipeline
}

void GxmGraphicsBackend::ConfigureView(GraphicsDeviceHandle device,
                                       uint16_t viewId,
                                       const ViewClearConfig& clearConfig) {
    std::cout << "GXM: Configuring view " << viewId << std::endl;
    (void)device;
    (void)clearConfig;
}

void GxmGraphicsBackend::Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
                              GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
                              uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset,
                              const std::array<float, 16>& modelMatrix) {
    std::cout << "GXM: Drawing " << indexCount << " indices (indexOffset=" << indexOffset
              << ", vertexOffset=" << vertexOffset << ")" << std::endl;

    if (!pipeline || !vertexBuffer || !indexBuffer) {
        std::cerr << "Invalid pipeline or buffer handles" << std::endl;
        return;
    }

    struct PipelineHandle {
        SceGxmVertexProgram* vertexProgram;
        SceGxmFragmentProgram* fragmentProgram;
    };
    struct BufferHandle {
        void* data;
        SceUID uid;
    };

    PipelineHandle* pipeHandle = static_cast<PipelineHandle*>(pipeline);
    BufferHandle* vbHandle = static_cast<BufferHandle*>(vertexBuffer);
    BufferHandle* ibHandle = static_cast<BufferHandle*>(indexBuffer);

    // Set vertex and fragment programs
    sceGxmSetVertexProgram(context_, pipeHandle->vertexProgram);
    sceGxmSetFragmentProgram(context_, pipeHandle->fragmentProgram);

    // Reserve uniform buffer and set matrices
    void* uniformBuffer;
    int err = sceGxmReserveVertexDefaultUniformBuffer(context_, &uniformBuffer);
    if (err == SCE_OK) {
        // Set model matrix (simplified - would need proper shader parameter lookup)
        sceGxmSetUniformDataF(uniformBuffer, nullptr, 0, 16, modelMatrix.data());
    }

    // Set vertex stream with base vertex offset
    const auto vertexStride = static_cast<uint32_t>(sizeof(core::Vertex));
    uint8_t* vertexBase = static_cast<uint8_t*>(vbHandle->data);
    vertexBase += static_cast<size_t>(vertexOffset) * vertexStride;
    sceGxmSetVertexStream(context_, 0, vertexBase);

    // Draw
    uint8_t* indexBase = static_cast<uint8_t*>(ibHandle->data);
    indexBase += static_cast<size_t>(indexOffset) * sizeof(uint16_t);
    err = sceGxmDraw(context_, SCE_GXM_PRIMITIVE_TRIANGLES, SCE_GXM_INDEX_FORMAT_U16, indexBase, indexCount);
    if (err != SCE_OK) {
        std::cerr << "Draw failed: " << err << std::endl;
    }
}

GraphicsDeviceHandle GxmGraphicsBackend::GetPhysicalDevice() const {
    return nullptr;
}

std::pair<uint32_t, uint32_t> GxmGraphicsBackend::GetSwapchainExtent() const {
    return {displayWidth_, displayHeight_};
}

uint32_t GxmGraphicsBackend::GetSwapchainFormat() const {
    return 0;
}

void* GxmGraphicsBackend::GetCurrentCommandBuffer() const {
    return nullptr;
}

void* GxmGraphicsBackend::GetGraphicsQueue() const {
    return nullptr;
}

// Helper methods

int GxmGraphicsBackend::createDisplayBuffers() {
    for (unsigned int i = 0; i < DISPLAY_BUFFER_COUNT; ++i) {
        displayBufferUid_[i] = sceKernelAllocMemBlock("displayBuffer", SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW,
                                                     DISPLAY_STRIDE_IN_PIXELS * DISPLAY_HEIGHT * 4, nullptr);
        if (displayBufferUid_[i] < 0) {
            return displayBufferUid_[i];
        }

        int err = sceKernelGetMemBlockBase(displayBufferUid_[i], &displayBufferData_[i]);
        if (err != SCE_OK) {
            return err;
        }

        // Create sync object
        err = sceGxmSyncObjectCreate(&displayBufferSync_[i]);
        if (err != SCE_OK) {
            return err;
        }
    }

    return SCE_OK;
}

void GxmGraphicsBackend::destroyDisplayBuffers() {
    for (unsigned int i = 0; i < DISPLAY_BUFFER_COUNT; ++i) {
        if (displayBufferSync_[i]) {
            sceGxmSyncObjectDestroy(displayBufferSync_[i]);
            displayBufferSync_[i] = nullptr;
        }
        if (displayBufferUid_[i] > 0) {
            sceKernelFreeMemBlock(displayBufferUid_[i]);
            displayBufferUid_[i] = 0;
        }
        displayBufferData_[i] = nullptr;
    }
}

SceGxmShaderPatcherId GxmGraphicsBackend::loadShader(const std::string& shaderPath, bool isVertex) {
    // For now, return invalid ID - would need to load actual shader binaries
    // In a real implementation, this would load .gxp files compiled for Vita
    return nullptr;
}

int GxmGraphicsBackend::createShaderPrograms(const std::string& shaderKey, const ShaderPaths& shaderPaths) {
    // For now, create minimal shader programs - would need actual shader loading
    // This is a placeholder implementation
    
    // Load vertex shader
    SceGxmShaderPatcherId vertexId = loadShader(shaderPaths.vertex, true);
    if (vertexId == nullptr) {
        return -1;
    }
    vertexShaderIds_[shaderKey] = vertexId;

    // Load fragment shader  
    SceGxmShaderPatcherId fragmentId = loadShader(shaderPaths.fragment, false);
    if (fragmentId == nullptr) {
        return -1;
    }
    fragmentShaderIds_[shaderKey] = fragmentId;

    // Create vertex program (simplified)
    SceGxmVertexProgram* vertexProgram;
    int err = sceGxmShaderPatcherCreateVertexProgram(shaderPatcher_, vertexId, nullptr, 0, nullptr, 0, &vertexProgram);
    if (err != SCE_OK) {
        return err;
    }
    vertexPrograms_[shaderKey] = vertexProgram;

    // Create fragment program (simplified)
    SceGxmFragmentProgram* fragmentProgram;
    err = sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher_, fragmentId, SCE_GXM_OUTPUT_REGISTER_FORMAT_UCHAR4,
                                                  SCE_GXM_MULTISAMPLE_NONE, nullptr, nullptr, &fragmentProgram);
    if (err != SCE_OK) {
        sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher_, vertexProgram);
        return err;
    }
    fragmentPrograms_[shaderKey] = fragmentProgram;

    return SCE_OK;
}

void GxmGraphicsBackend::destroyShaderPrograms() {
    for (auto& pair : fragmentPrograms_) {
        if (pair.second) {
            sceGxmShaderPatcherReleaseFragmentProgram(shaderPatcher_, pair.second);
        }
    }
    fragmentPrograms_.clear();

    for (auto& pair : vertexPrograms_) {
        if (pair.second) {
            sceGxmShaderPatcherReleaseVertexProgram(shaderPatcher_, pair.second);
        }
    }
    vertexPrograms_.clear();

    vertexShaderIds_.clear();
    fragmentShaderIds_.clear();
}

}  // namespace sdl3cpp::services::impl

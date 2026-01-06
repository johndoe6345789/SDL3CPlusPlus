#include "bgfx_graphics_backend.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <bgfx/platform.h>
#include <shaderc/shaderc.hpp>

#include <algorithm>
#include <cstdint>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iterator>
#include <string>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::services::impl {
namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

glm::mat4 ToMat4(const std::array<float, 16>& value) {
    return glm::make_mat4(value.data());
}

bool IsIdentityMatrix(const std::array<float, 16>& value) {
    const float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    for (size_t i = 0; i < 16; ++i) {
        if (value[i] != identity[i]) {
            return false;
        }
    }
    return true;
}

void SetUniformIfValid(bgfx::UniformHandle handle, const void* data, uint16_t count = 1) {
    if (bgfx::isValid(handle)) {
        bgfx::setUniform(handle, data, count);
    }
}

bgfx::RendererType::Enum RendererFromString(const std::string& value) {
    const std::string lower = ToLower(value);
    if (lower == "vulkan") {
        return bgfx::RendererType::Vulkan;
    }
    if (lower == "auto") {
        return bgfx::RendererType::Count;
    }
    return bgfx::RendererType::Vulkan;
}

std::string RendererTypeName(bgfx::RendererType::Enum type) {
    if (type == bgfx::RendererType::Count) {
        return "auto";
    }
    const char* name = bgfx::getRendererName(type);
    if (!name || name[0] == '\0') {
        return "unknown";
    }
    return name;
}

std::vector<bgfx::RendererType::Enum> GetSupportedRenderers() {
    const uint8_t count = bgfx::getSupportedRenderers();
    std::vector<bgfx::RendererType::Enum> renderers;
    renderers.resize(count);
    if (count > 0) {
        bgfx::getSupportedRenderers(count, renderers.data());
    }
    return renderers;
}

std::string JoinRendererNames(const std::vector<bgfx::RendererType::Enum>& renderers) {
    if (renderers.empty()) {
        return "none";
    }
    std::string result;
    for (size_t i = 0; i < renderers.size(); ++i) {
        if (i > 0) {
            result += ", ";
        }
        result += RendererTypeName(renderers[i]);
    }
    return result;
}

}  // namespace

BgfxGraphicsBackend::BgfxGraphicsBackend(std::shared_ptr<IConfigService> configService,
                                         std::shared_ptr<ILogger> logger)
    : configService_(std::move(configService)),
      logger_(std::move(logger)) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "BgfxGraphicsBackend",
                       "configService=" + std::string(configService_ ? "set" : "null"));
    }
    vertexLayout_.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)
        .end();

    const std::array<float, 16> identity = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    viewState_.view = identity;
    viewState_.proj = identity;
    viewState_.viewProj = identity;
    viewState_.cameraPosition = {0.0f, 0.0f, 0.0f};
}

BgfxGraphicsBackend::~BgfxGraphicsBackend() {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "~BgfxGraphicsBackend");
    }
    if (initialized_) {
        Shutdown();
    }
}

void BgfxGraphicsBackend::SetupPlatformData(void* window) {
    bgfx::PlatformData pd{};
    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window);
    if (!sdlWindow) {
        bgfx::setPlatformData(pd);
        return;
    }

    SDL_PropertiesID props = SDL_GetWindowProperties(sdlWindow);
#if defined(_WIN32)
    pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__APPLE__)
    pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#elif defined(__linux__)
    void* wlDisplay = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    void* wlSurface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    if (wlDisplay && wlSurface) {
        pd.ndt = wlDisplay;
        pd.nwh = wlSurface;
    } else {
        void* x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
        Sint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
        if (x11Display && x11Window != 0) {
            pd.ndt = x11Display;
            pd.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(x11Window));
        }
    }
#else
    pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif
    bgfx::setPlatformData(pd);
}

bgfx::RendererType::Enum BgfxGraphicsBackend::ResolveRendererType() const {
    if (!configService_) {
        return bgfx::RendererType::Vulkan;
    }
    const auto& config = configService_->GetBgfxConfig();
    return RendererFromString(config.renderer);
}

void BgfxGraphicsBackend::Initialize(void* window, const GraphicsConfig& config) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize");
    }
    if (initialized_) {
        return;
    }
    (void)config;

    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window);
    int width = 0;
    int height = 0;
    if (sdlWindow) {
        SDL_GetWindowSizeInPixels(sdlWindow, &width, &height);
    }
    viewportWidth_ = static_cast<uint32_t>(std::max(1, width));
    viewportHeight_ = static_cast<uint32_t>(std::max(1, height));

    SetupPlatformData(window);

    const auto requestedRenderer = ResolveRendererType();
    const auto supportedRenderers = GetSupportedRenderers();
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "requestedRenderer=" + RendererTypeName(requestedRenderer) +
                       ", supportedRenderers=" + JoinRendererNames(supportedRenderers));
    }

    bgfx::Init init{};
    init.resolution.width = viewportWidth_;
    init.resolution.height = viewportHeight_;
    init.resolution.reset = BGFX_RESET_VSYNC;

    std::vector<bgfx::RendererType::Enum> candidates;
    auto addCandidate = [&candidates](bgfx::RendererType::Enum type) {
        if (std::find(candidates.begin(), candidates.end(), type) == candidates.end()) {
            candidates.push_back(type);
        }
    };

    if (requestedRenderer == bgfx::RendererType::Count) {
        addCandidate(bgfx::RendererType::Count);
    } else {
        addCandidate(requestedRenderer);
    }
    for (bgfx::RendererType::Enum renderer : supportedRenderers) {
        addCandidate(renderer);
    }
    addCandidate(bgfx::RendererType::Count);

    bool initialized = false;
    for (bgfx::RendererType::Enum renderer : candidates) {
        init.type = renderer;
        if (logger_) {
            logger_->Trace("BgfxGraphicsBackend", "Initialize",
                           "attemptingRenderer=" + RendererTypeName(renderer));
        }
        if (bgfx::init(init)) {
            initialized = true;
            break;
        }
        if (logger_) {
            logger_->Warn("BgfxGraphicsBackend::Initialize: bgfx init failed for renderer=" +
                          RendererTypeName(renderer));
        }
    }

    if (!initialized) {
        throw std::runtime_error("Failed to initialize bgfx");
    }
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "selectedRenderer=" + RendererTypeName(bgfx::getRendererType()));
    }

    bgfx::setViewClear(viewId_, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x1f1f1fff, 1.0f, 0);
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    InitializeUniforms();

    initialized_ = true;
}

void BgfxGraphicsBackend::Shutdown() {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Shutdown");
    }
    if (!initialized_) {
        return;
    }

    DestroyPipelines();
    DestroyBuffers();
    DestroyUniforms();
    bgfx::shutdown();
    initialized_ = false;
}

void BgfxGraphicsBackend::RecreateSwapchain(uint32_t width, uint32_t height) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "RecreateSwapchain",
                       "width=" + std::to_string(width) +
                       ", height=" + std::to_string(height));
    }
    if (!initialized_) {
        return;
    }
    if (width == 0 || height == 0) {
        return;
    }
    viewportWidth_ = width;
    viewportHeight_ = height;
    bgfx::reset(viewportWidth_, viewportHeight_, BGFX_RESET_VSYNC);
}

void BgfxGraphicsBackend::WaitIdle() {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "WaitIdle");
    }
}

GraphicsDeviceHandle BgfxGraphicsBackend::CreateDevice() {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "CreateDevice");
    }
    return reinterpret_cast<GraphicsDeviceHandle>(1);
}

void BgfxGraphicsBackend::DestroyDevice(GraphicsDeviceHandle device) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "DestroyDevice");
    }
}

std::vector<uint8_t> BgfxGraphicsBackend::ReadShaderSource(const std::string& path,
                                                           const std::string& source) const {
    if (!source.empty()) {
        return std::vector<uint8_t>(source.begin(), source.end());
    }
    if (path.empty()) {
        throw std::runtime_error("Shader path and source are empty");
    }
    std::filesystem::path shaderPath(path);
    if (shaderPath.extension() == ".spv") {
        shaderPath.replace_extension();
    }
    if (!std::filesystem::exists(shaderPath)) {
        throw std::runtime_error("Shader file not found: " + shaderPath.string());
    }
    std::ifstream sourceFile(shaderPath);
    if (!sourceFile) {
        throw std::runtime_error("Failed to open shader source: " + shaderPath.string());
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(sourceFile)),
                                std::istreambuf_iterator<char>());
}

bgfx::ShaderHandle BgfxGraphicsBackend::CreateShader(const std::string& label,
                                                     const std::string& source,
                                                     bool isVertex) const {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_2);

    shaderc_shader_kind kind = isVertex ? shaderc_vertex_shader : shaderc_fragment_shader;

    auto result = compiler.CompileGlslToSpv(source, kind, label.c_str(), options);
    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string error = result.GetErrorMessage();
        if (logger_) {
            logger_->Error("Bgfx shader compilation failed: " + label + "\n" + error);
        }
        throw std::runtime_error("Bgfx shader compilation failed: " + label + "\n" + error);
    }

    std::vector<uint32_t> spirv(result.cbegin(), result.cend());
    const bgfx::Memory* mem = bgfx::copy(spirv.data(),
                                         static_cast<uint32_t>(spirv.size() * sizeof(uint32_t)));
    return bgfx::createShader(mem);
}

void BgfxGraphicsBackend::InitializeUniforms() {
    materialXUniforms_.worldMatrix = bgfx::createUniform("u_worldMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.viewMatrix = bgfx::createUniform("u_viewMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.projectionMatrix = bgfx::createUniform("u_projectionMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.viewProjectionMatrix = bgfx::createUniform("u_viewProjectionMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.worldViewMatrix = bgfx::createUniform("u_worldViewMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.worldViewProjectionMatrix = bgfx::createUniform("u_worldViewProjectionMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.worldInverseTransposeMatrix = bgfx::createUniform("u_worldInverseTransposeMatrix", bgfx::UniformType::Mat4);
    materialXUniforms_.viewPosition = bgfx::createUniform("u_viewPosition", bgfx::UniformType::Vec4);
}

void BgfxGraphicsBackend::DestroyUniforms() {
    bgfx::UniformHandle handles[] = {
        materialXUniforms_.worldMatrix,
        materialXUniforms_.viewMatrix,
        materialXUniforms_.projectionMatrix,
        materialXUniforms_.viewProjectionMatrix,
        materialXUniforms_.worldViewMatrix,
        materialXUniforms_.worldViewProjectionMatrix,
        materialXUniforms_.worldInverseTransposeMatrix,
        materialXUniforms_.viewPosition
    };
    for (bgfx::UniformHandle handle : handles) {
        if (bgfx::isValid(handle)) {
            bgfx::destroy(handle);
        }
    }
    materialXUniforms_ = MaterialXUniforms{};
}

void BgfxGraphicsBackend::ApplyMaterialXUniforms(const std::array<float, 16>& modelMatrix) {
    glm::mat4 model = ToMat4(modelMatrix);
    glm::mat4 view = ToMat4(viewState_.view);
    glm::mat4 proj = ToMat4(viewState_.proj);
    glm::mat4 viewProj = (IsIdentityMatrix(viewState_.view) && IsIdentityMatrix(viewState_.proj))
        ? ToMat4(viewState_.viewProj)
        : proj * view;
    glm::mat4 worldView = view * model;
    glm::mat4 worldViewProj = viewProj * model;
    glm::mat4 worldInverseTranspose = glm::transpose(glm::inverse(model));

    SetUniformIfValid(materialXUniforms_.worldMatrix, glm::value_ptr(model));
    SetUniformIfValid(materialXUniforms_.viewMatrix, glm::value_ptr(view));
    SetUniformIfValid(materialXUniforms_.projectionMatrix, glm::value_ptr(proj));
    SetUniformIfValid(materialXUniforms_.viewProjectionMatrix, glm::value_ptr(viewProj));
    SetUniformIfValid(materialXUniforms_.worldViewMatrix, glm::value_ptr(worldView));
    SetUniformIfValid(materialXUniforms_.worldViewProjectionMatrix, glm::value_ptr(worldViewProj));
    SetUniformIfValid(materialXUniforms_.worldInverseTransposeMatrix, glm::value_ptr(worldInverseTranspose));

    float viewPosition[4] = {
        viewState_.cameraPosition[0],
        viewState_.cameraPosition[1],
        viewState_.cameraPosition[2],
        1.0f
    };
    SetUniformIfValid(materialXUniforms_.viewPosition, viewPosition);
}

GraphicsPipelineHandle BgfxGraphicsBackend::CreatePipeline(GraphicsDeviceHandle device,
                                                           const std::string& shaderKey,
                                                           const ShaderPaths& shaderPaths) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "CreatePipeline", "shaderKey=" + shaderKey);
    }
    std::vector<uint8_t> vertexBytes = ReadShaderSource(shaderPaths.vertex, shaderPaths.vertexSource);
    std::vector<uint8_t> fragmentBytes = ReadShaderSource(shaderPaths.fragment, shaderPaths.fragmentSource);

    std::string vertexSource(vertexBytes.begin(), vertexBytes.end());
    std::string fragmentSource(fragmentBytes.begin(), fragmentBytes.end());

    bgfx::ShaderHandle vs = CreateShader(shaderKey + ":vertex", vertexSource, true);
    bgfx::ShaderHandle fs = CreateShader(shaderKey + ":fragment", fragmentSource, false);
    bgfx::ProgramHandle program = bgfx::createProgram(vs, fs, true);

    auto entry = std::make_unique<PipelineEntry>();
    entry->program = program;
    GraphicsPipelineHandle handle = reinterpret_cast<GraphicsPipelineHandle>(entry.get());
    pipelines_.emplace(handle, std::move(entry));
    return handle;
}

void BgfxGraphicsBackend::DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "DestroyPipeline");
    }
    auto it = pipelines_.find(pipeline);
    if (it == pipelines_.end()) {
        return;
    }
    if (bgfx::isValid(it->second->program)) {
        bgfx::destroy(it->second->program);
    }
    pipelines_.erase(it);
}

GraphicsBufferHandle BgfxGraphicsBackend::CreateVertexBuffer(GraphicsDeviceHandle device,
                                                            const std::vector<uint8_t>& data) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "CreateVertexBuffer",
                       "data.size=" + std::to_string(data.size()));
    }
    if (data.empty() || data.size() % sizeof(core::Vertex) != 0) {
        throw std::runtime_error("Vertex data invalid for bgfx");
    }
    uint32_t vertexCount = static_cast<uint32_t>(data.size() / sizeof(core::Vertex));
    const bgfx::Memory* mem = bgfx::copy(data.data(), static_cast<uint32_t>(data.size()));
    bgfx::VertexBufferHandle buffer = bgfx::createVertexBuffer(mem, vertexLayout_);

    auto entry = std::make_unique<VertexBufferEntry>();
    entry->handle = buffer;
    entry->vertexCount = vertexCount;
    GraphicsBufferHandle handle = reinterpret_cast<GraphicsBufferHandle>(entry.get());
    vertexBuffers_.emplace(handle, std::move(entry));
    return handle;
}

GraphicsBufferHandle BgfxGraphicsBackend::CreateIndexBuffer(GraphicsDeviceHandle device,
                                                           const std::vector<uint8_t>& data) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "CreateIndexBuffer",
                       "data.size=" + std::to_string(data.size()));
    }
    if (data.empty() || data.size() % sizeof(uint16_t) != 0) {
        throw std::runtime_error("Index data invalid for bgfx");
    }
    uint32_t indexCount = static_cast<uint32_t>(data.size() / sizeof(uint16_t));
    const bgfx::Memory* mem = bgfx::copy(data.data(), static_cast<uint32_t>(data.size()));
    bgfx::IndexBufferHandle buffer = bgfx::createIndexBuffer(mem);

    auto entry = std::make_unique<IndexBufferEntry>();
    entry->handle = buffer;
    entry->indexCount = indexCount;
    GraphicsBufferHandle handle = reinterpret_cast<GraphicsBufferHandle>(entry.get());
    indexBuffers_.emplace(handle, std::move(entry));
    return handle;
}

void BgfxGraphicsBackend::DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "DestroyBuffer");
    }
    auto vIt = vertexBuffers_.find(buffer);
    if (vIt != vertexBuffers_.end()) {
        if (bgfx::isValid(vIt->second->handle)) {
            bgfx::destroy(vIt->second->handle);
        }
        vertexBuffers_.erase(vIt);
        return;
    }
    auto iIt = indexBuffers_.find(buffer);
    if (iIt != indexBuffers_.end()) {
        if (bgfx::isValid(iIt->second->handle)) {
            bgfx::destroy(iIt->second->handle);
        }
        indexBuffers_.erase(iIt);
    }
}

bool BgfxGraphicsBackend::BeginFrame(GraphicsDeviceHandle device) {
    if (!initialized_) {
        return false;
    }
    bgfx::setViewRect(viewId_, 0, 0, viewportWidth_, viewportHeight_);
    bgfx::touch(viewId_);
    return true;
}

bool BgfxGraphicsBackend::EndFrame(GraphicsDeviceHandle device) {
    if (!initialized_) {
        return false;
    }
    bgfx::frame();
    return true;
}

void BgfxGraphicsBackend::SetViewState(const ViewState& viewState) {
    viewState_ = viewState;
    bgfx::setViewTransform(viewId_, viewState_.view.data(), viewState_.proj.data());
}

void BgfxGraphicsBackend::Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
                               GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
                               uint32_t indexOffset, uint32_t indexCount, int32_t vertexOffset,
                               const std::array<float, 16>& modelMatrix) {
    auto pipelineIt = pipelines_.find(pipeline);
    if (pipelineIt == pipelines_.end()) {
        if (logger_) {
            logger_->Error("BgfxGraphicsBackend::Draw: Pipeline not found");
        }
        return;
    }
    auto vertexIt = vertexBuffers_.find(vertexBuffer);
    auto indexIt = indexBuffers_.find(indexBuffer);
    if (vertexIt == vertexBuffers_.end() || indexIt == indexBuffers_.end()) {
        if (logger_) {
            logger_->Error("BgfxGraphicsBackend::Draw: Buffer handles not found");
        }
        return;
    }
    const auto& vb = vertexIt->second;
    const auto& ib = indexIt->second;

    uint32_t startVertex = static_cast<uint32_t>(std::max(0, vertexOffset));
    uint32_t availableVertices = vb->vertexCount > startVertex
        ? vb->vertexCount - startVertex
        : 0;
    if (availableVertices == 0) {
        return;
    }

    bgfx::setTransform(modelMatrix.data());
    ApplyMaterialXUniforms(modelMatrix);
    bgfx::setVertexBuffer(0, vb->handle, startVertex, availableVertices);
    bgfx::setIndexBuffer(ib->handle, indexOffset, indexCount);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z |
                   BGFX_STATE_DEPTH_TEST_LESS | BGFX_STATE_CULL_CW | BGFX_STATE_MSAA);
    bgfx::submit(viewId_, pipelineIt->second->program);
}

GraphicsDeviceHandle BgfxGraphicsBackend::GetPhysicalDevice() const {
    return nullptr;
}

std::pair<uint32_t, uint32_t> BgfxGraphicsBackend::GetSwapchainExtent() const {
    return {viewportWidth_, viewportHeight_};
}

uint32_t BgfxGraphicsBackend::GetSwapchainFormat() const {
    return 0;
}

void* BgfxGraphicsBackend::GetCurrentCommandBuffer() const {
    return nullptr;
}

void* BgfxGraphicsBackend::GetGraphicsQueue() const {
    return nullptr;
}

void BgfxGraphicsBackend::DestroyPipelines() {
    for (auto& [handle, entry] : pipelines_) {
        if (bgfx::isValid(entry->program)) {
            bgfx::destroy(entry->program);
        }
    }
    pipelines_.clear();
}

void BgfxGraphicsBackend::DestroyBuffers() {
    for (auto& [handle, entry] : vertexBuffers_) {
        if (bgfx::isValid(entry->handle)) {
            bgfx::destroy(entry->handle);
        }
    }
    vertexBuffers_.clear();
    for (auto& [handle, entry] : indexBuffers_) {
        if (bgfx::isValid(entry->handle)) {
            bgfx::destroy(entry->handle);
        }
    }
    indexBuffers_.clear();
}

}  // namespace sdl3cpp::services::impl

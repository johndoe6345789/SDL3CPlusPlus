#include "bgfx_graphics_backend.hpp"
#include "../interfaces/i_pipeline_compiler_service.hpp"
#include <stb_image.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_properties.h>
#include <SDL3/SDL_video.h>
#include <bgfx/platform.h>

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
#include <optional>
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

uint64_t DefaultSamplerFlags() {
    // Repeat + linear are the default sampler state in bgfx; no flags needed.
    return 0;
}

bgfx::TextureHandle CreateSolidTexture(uint32_t rgba, uint64_t flags) {
    const bgfx::Memory* mem = bgfx::copy(&rgba, sizeof(rgba));
    return bgfx::createTexture2D(1, 1, false, 1, bgfx::TextureFormat::RGBA8, flags, mem);
}

void SetUniformIfValid(bgfx::UniformHandle handle, const void* data, uint16_t count = 1) {
    if (bgfx::isValid(handle)) {
        bgfx::setUniform(handle, data, count);
    }
}

bool TryParseRendererType(const std::string& value, bgfx::RendererType::Enum& out) {
    const std::string lower = ToLower(value);
    if (lower == "auto") {
        out = bgfx::RendererType::Count;
        return true;
    }
    if (lower == "vulkan") {
        out = bgfx::RendererType::Vulkan;
        return true;
    }
    if (lower == "opengl") {
        out = bgfx::RendererType::OpenGL;
        return true;
    }
    if (lower == "opengles" || lower == "opengles2") {
        out = bgfx::RendererType::OpenGLES;
        return true;
    }
    if (lower == "direct3d11" || lower == "d3d11") {
        out = bgfx::RendererType::Direct3D11;
        return true;
    }
    if (lower == "direct3d12" || lower == "d3d12") {
        out = bgfx::RendererType::Direct3D12;
        return true;
    }
    if (lower == "metal") {
        out = bgfx::RendererType::Metal;
        return true;
    }
    if (lower == "noop") {
        out = bgfx::RendererType::Noop;
        return true;
    }
    return false;
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

const char* HandleTypeName(bgfx::NativeWindowHandleType::Enum type) {
    switch (type) {
        case bgfx::NativeWindowHandleType::Default:
            return "default";
        case bgfx::NativeWindowHandleType::Wayland:
            return "wayland";
        case bgfx::NativeWindowHandleType::Count:
            return "count";
        default:
            return "unknown";
    }
}

const char* RendererConfigName(bgfx::RendererType::Enum type) {
    switch (type) {
        case bgfx::RendererType::Vulkan:
            return "vulkan";
        case bgfx::RendererType::OpenGL:
            return "opengl";
        case bgfx::RendererType::OpenGLES:
            return "opengles";
        case bgfx::RendererType::Direct3D11:
            return "direct3d11";
        case bgfx::RendererType::Direct3D12:
            return "direct3d12";
        case bgfx::RendererType::Metal:
            return "metal";
        case bgfx::RendererType::Noop:
            return "noop";
        case bgfx::RendererType::Count:
        default:
            return "auto";
    }
}

bool IsNoopRenderer(bgfx::RendererType::Enum type) {
    return type == bgfx::RendererType::Noop;
}

bool ContainsRenderer(const std::vector<bgfx::RendererType::Enum>& renderers,
                      bgfx::RendererType::Enum type) {
    return std::find(renderers.begin(), renderers.end(), type) != renderers.end();
}

bool IsRendererAllowedOnPlatform(bgfx::RendererType::Enum type, const std::string& platformName) {
    if (type == bgfx::RendererType::Count || type == bgfx::RendererType::Noop) {
        return true;
    }
    const std::string platformLower = ToLower(platformName);
    if (platformLower.find("windows") != std::string::npos) {
        return type == bgfx::RendererType::Direct3D11 ||
               type == bgfx::RendererType::Direct3D12 ||
               type == bgfx::RendererType::Vulkan ||
               type == bgfx::RendererType::OpenGL ||
               type == bgfx::RendererType::OpenGLES;
    }
    if (platformLower.find("mac") != std::string::npos ||
        platformLower.find("darwin") != std::string::npos) {
        return type == bgfx::RendererType::Metal ||
               type == bgfx::RendererType::OpenGL ||
               type == bgfx::RendererType::Vulkan;
    }
    return type == bgfx::RendererType::Vulkan ||
           type == bgfx::RendererType::OpenGL ||
           type == bgfx::RendererType::OpenGLES;
}

void AddRendererIfSupported(std::vector<bgfx::RendererType::Enum>& ordered,
                            const std::vector<bgfx::RendererType::Enum>& supported,
                            bgfx::RendererType::Enum type) {
    if (IsNoopRenderer(type) || type == bgfx::RendererType::Count) {
        return;
    }
    if (!ContainsRenderer(supported, type)) {
        return;
    }
    if (!ContainsRenderer(ordered, type)) {
        ordered.push_back(type);
    }
}

std::vector<bgfx::RendererType::Enum> BuildPreferredRenderers(
    const std::vector<bgfx::RendererType::Enum>& supportedRenderers,
    const std::string& platformName,
    const std::string& videoDriverName) {
    std::vector<bgfx::RendererType::Enum> preferred;
    const std::string platformLower = ToLower(platformName);
    const std::string videoLower = ToLower(videoDriverName);

    if (platformLower.find("windows") != std::string::npos) {
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Direct3D12);
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Direct3D11);
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Vulkan);
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::OpenGL);
    } else if (platformLower.find("mac") != std::string::npos ||
               platformLower.find("darwin") != std::string::npos) {
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Metal);
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::OpenGL);
        AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Vulkan);
    } else {
        const bool waylandOrX11 = (videoLower == "wayland") || (videoLower == "x11");
        const bool kmsdrm = (videoLower == "kmsdrm");
        if (waylandOrX11 || kmsdrm) {
            AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Vulkan);
            AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::OpenGL);
            AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::OpenGLES);
        } else {
            AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::Vulkan);
            AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::OpenGL);
            AddRendererIfSupported(preferred, supportedRenderers, bgfx::RendererType::OpenGLES);
        }
    }

    return preferred;
}

std::optional<bgfx::RendererType::Enum> RecommendFallbackRenderer(
    bgfx::RendererType::Enum requested,
    const std::vector<bgfx::RendererType::Enum>& supportedRenderers,
    const std::string& platformName,
    const std::string& videoDriverName) {
    const auto preferred = BuildPreferredRenderers(supportedRenderers, platformName, videoDriverName);
    for (bgfx::RendererType::Enum type : preferred) {
        if (type != requested) {
            return type;
        }
    }
    return std::nullopt;
}

}  // namespace

BgfxGraphicsBackend::BgfxGraphicsBackend(std::shared_ptr<IConfigService> configService,
                                         std::shared_ptr<IPlatformService> platformService,
                                         std::shared_ptr<ILogger> logger,
                                         std::shared_ptr<IPipelineCompilerService> pipelineCompiler)
    : configService_(std::move(configService)),
      platformService_(std::move(platformService)),
      logger_(std::move(logger)),
      pipelineCompiler_(std::move(pipelineCompiler)) {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "BgfxGraphicsBackend",
                       "configService=" + std::string(configService_ ? "set" : "null") +
                       ", platformService=" + std::string(platformService_ ? "set" : "null"));
    }
    vertexLayout_.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)  // location 0
        .add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float)    // location 1
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float) // location 2
        .add(bgfx::Attrib::Color0, 3, bgfx::AttribType::Float)    // location 3
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
    platformData_ = bgfx::PlatformData{};
    auto& pd = platformData_;
    platformHandleInfo_ = PlatformHandleInfo{};
    platformHandleInfo_.handleType = bgfx::NativeWindowHandleType::Default;
    SDL_Window* sdlWindow = static_cast<SDL_Window*>(window);
    if (!sdlWindow) {
        if (logger_) {
            logger_->Trace("BgfxGraphicsBackend", "SetupPlatformData", "windowIsNull=true");
        }
        platformHandleInfo_.hasWindowHandle = false;
        platformHandleInfo_.hasDisplayHandle = false;
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
    void* x11Display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    Sint64 x11Window = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    const bool hasWayland = wlDisplay && wlSurface;
    const bool hasX11 = x11Display && x11Window != 0;
    platformHandleInfo_.hasWayland = hasWayland;
    platformHandleInfo_.hasX11 = hasX11;
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "SetupPlatformData",
                       "waylandAvailable=" + std::string(hasWayland ? "true" : "false") +
                       ", x11Available=" + std::string(hasX11 ? "true" : "false"));
    }
    if (hasWayland) {
        pd.ndt = wlDisplay;
        pd.nwh = wlSurface;
        pd.type = bgfx::NativeWindowHandleType::Wayland;
        platformHandleInfo_.handleType = bgfx::NativeWindowHandleType::Wayland;
        if (logger_) {
            logger_->Trace("BgfxGraphicsBackend", "SetupPlatformData", "selectedHandleType=Wayland");
        }
    } else if (hasX11) {
        pd.ndt = x11Display;
        pd.nwh = reinterpret_cast<void*>(static_cast<uintptr_t>(x11Window));
        pd.type = bgfx::NativeWindowHandleType::Default;
        platformHandleInfo_.handleType = bgfx::NativeWindowHandleType::Default;
        if (logger_) {
            logger_->Trace("BgfxGraphicsBackend", "SetupPlatformData", "selectedHandleType=X11");
        }
    }
#else
    pd.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#endif
    platformHandleInfo_.hasWindowHandle = pd.nwh != nullptr;
    platformHandleInfo_.hasDisplayHandle = pd.ndt != nullptr;
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "SetupPlatformData",
                       "nwh=" + std::to_string(reinterpret_cast<uintptr_t>(pd.nwh)) +
                       ", ndt=" + std::to_string(reinterpret_cast<uintptr_t>(pd.ndt)) +
                       ", handleType=" + std::string(HandleTypeName(platformHandleInfo_.handleType)));
    }
    bgfx::setPlatformData(pd);
}

bgfx::RendererType::Enum BgfxGraphicsBackend::ResolveRendererType() const {
    if (!configService_) {
        return bgfx::RendererType::Vulkan;
    }
    const auto& config = configService_->GetBgfxConfig();
    bgfx::RendererType::Enum resolved = bgfx::RendererType::Vulkan;
    const bool parsed = TryParseRendererType(config.renderer, resolved);
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "ResolveRendererType",
                       "renderer=" + config.renderer +
                       ", resolved=" + RendererTypeName(resolved) +
                       ", parsed=" + std::string(parsed ? "true" : "false"));
    }
    if (!parsed && logger_) {
        logger_->Warn("BgfxGraphicsBackend::ResolveRendererType: Unknown renderer '" +
                      config.renderer + "', defaulting to Vulkan");
    }
    return resolved;
}

void BgfxGraphicsBackend::LogRendererFailureDetails(
    bgfx::RendererType::Enum renderer,
    const std::vector<bgfx::RendererType::Enum>& supportedRenderers,
    const std::string& platformName,
    const std::string& videoDriverName) {
    if (!logger_) {
        return;
    }

    logger_->Warn("BgfxGraphicsBackend::Initialize: Renderer " +
                  RendererTypeName(renderer) + " failed (platform=" +
                  platformName + ", videoDriver=" + videoDriverName +
                  ", handleType=" +
                  std::string(HandleTypeName(platformHandleInfo_.handleType)) +
                  ", windowHandle=" +
                  std::string(platformHandleInfo_.hasWindowHandle ? "set" : "null") +
                  ", displayHandle=" +
                  std::string(platformHandleInfo_.hasDisplayHandle ? "set" : "null") +
                  ", supportedRenderers=" + JoinRendererNames(supportedRenderers) + ")");

    const auto fallback = RecommendFallbackRenderer(renderer, supportedRenderers,
                                                   platformName, videoDriverName);
    if (fallback.has_value()) {
        logger_->Warn("BgfxGraphicsBackend::Initialize: Recommended fallback renderer=" +
                      RendererTypeName(fallback.value()) +
                      " (set bgfx.renderer=" + std::string(RendererConfigName(fallback.value())) + ")");
    }

    if (renderer == bgfx::RendererType::Vulkan) {
        if (!platformHandleInfo_.hasWindowHandle) {
            logger_->Warn("BgfxGraphicsBackend::Initialize: Vulkan requires a native window handle");
        }
        if (videoDriverName == "wayland" && !platformHandleInfo_.hasWayland) {
            logger_->Warn("BgfxGraphicsBackend::Initialize: SDL reports Wayland, but no Wayland "
                          "window handle was provided");
        }
        if (videoDriverName == "x11" && !platformHandleInfo_.hasX11) {
            logger_->Warn("BgfxGraphicsBackend::Initialize: SDL reports X11, but no X11 "
                          "window handle was provided");
        }
    }

    if (platformService_ && !loggedInitFailureDiagnostics_) {
        platformService_->LogSystemInfo();
        loggedInitFailureDiagnostics_ = true;
    }
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
    const std::string platformName = platformService_
        ? platformService_->GetPlatformName()
        : "unknown";
    const std::string videoDriverName = platformService_
        ? platformService_->GetCurrentVideoDriverName()
        : "unknown";
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "requestedRenderer=" + RendererTypeName(requestedRenderer) +
                       ", supportedRenderers=" + JoinRendererNames(supportedRenderers) +
                       ", platform=" + platformName +
                       ", videoDriver=" + videoDriverName);
    }
    if (requestedRenderer != bgfx::RendererType::Count &&
        !ContainsRenderer(supportedRenderers, requestedRenderer) &&
        logger_) {
        logger_->Warn("BgfxGraphicsBackend::Initialize: Requested renderer=" +
                      RendererTypeName(requestedRenderer) +
                      " is not in the supported renderers list");
    }
    if (requestedRenderer != bgfx::RendererType::Count &&
        !IsRendererAllowedOnPlatform(requestedRenderer, platformName) &&
        logger_) {
        logger_->Warn("BgfxGraphicsBackend::Initialize: Requested renderer=" +
                      RendererTypeName(requestedRenderer) +
                      " is not recommended for platform=" + platformName);
    }

    bgfx::Init init{};
    init.resolution.width = viewportWidth_;
    init.resolution.height = viewportHeight_;
    init.resolution.reset = BGFX_RESET_VSYNC;
    init.platformData = platformData_;
    init.debug = BGFX_DEBUG_TEXT;  // Enable debugging for shader validation
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "initPlatformData.nwh=" + std::to_string(reinterpret_cast<uintptr_t>(init.platformData.nwh)) +
                       ", initPlatformData.ndt=" + std::to_string(reinterpret_cast<uintptr_t>(init.platformData.ndt)) +
                       ", initPlatformData.type=" + std::string(HandleTypeName(init.platformData.type)));
    }

    std::vector<bgfx::RendererType::Enum> candidates;
    auto addCandidate = [&candidates](bgfx::RendererType::Enum type) {
        if (std::find(candidates.begin(), candidates.end(), type) == candidates.end()) {
            candidates.push_back(type);
        }
    };

    const auto preferredRenderers =
        BuildPreferredRenderers(supportedRenderers, platformName, videoDriverName);
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "preferredRenderers=" + JoinRendererNames(preferredRenderers));
    }

    if (requestedRenderer != bgfx::RendererType::Count) {
        addCandidate(requestedRenderer);
    }
    for (bgfx::RendererType::Enum renderer : preferredRenderers) {
        addCandidate(renderer);
    }
    for (bgfx::RendererType::Enum renderer : supportedRenderers) {
        if (!IsNoopRenderer(renderer) && renderer != bgfx::RendererType::Count &&
            IsRendererAllowedOnPlatform(renderer, platformName)) {
            addCandidate(renderer);
        }
    }
    addCandidate(bgfx::RendererType::Count);
    if (ContainsRenderer(supportedRenderers, bgfx::RendererType::Noop) ||
        requestedRenderer == bgfx::RendererType::Noop) {
        addCandidate(bgfx::RendererType::Noop);
    }
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "candidateRenderers=" + JoinRendererNames(candidates));
    }

    bool initialized = false;
    bool requestedFailed = false;
    const bool requestedExplicit = requestedRenderer != bgfx::RendererType::Count;
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
        if (renderer == requestedRenderer && !requestedFailed) {
            requestedFailed = true;
            LogRendererFailureDetails(renderer, supportedRenderers, platformName, videoDriverName);
        }
    }

    if (!initialized) {
        if (platformService_ && !loggedInitFailureDiagnostics_) {
            platformService_->LogSystemInfo();
            loggedInitFailureDiagnostics_ = true;
        }
        throw std::runtime_error("Failed to initialize bgfx");
    }
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Initialize",
                       "selectedRenderer=" + RendererTypeName(bgfx::getRendererType()));
    }
    if (requestedExplicit && bgfx::getRendererType() != requestedRenderer && logger_) {
        logger_->Warn("BgfxGraphicsBackend::Initialize: Requested renderer=" +
                      RendererTypeName(requestedRenderer) + ", using=" +
                      RendererTypeName(bgfx::getRendererType()));
    }
    if (bgfx::getRendererType() == bgfx::RendererType::Noop && logger_) {
        logger_->Warn("BgfxGraphicsBackend::Initialize: Noop renderer selected; rendering disabled");
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
    const bgfx::RendererType::Enum rendererType = bgfx::getRendererType();
    
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "CreateShader",
                       "label=" + label +
                           ", renderer=" + RendererTypeName(rendererType) +
                           ", sourceLength=" + std::to_string(source.size()));
    }

    // For OpenGL, bgfx expects raw GLSL source (no wrapper needed)
    // For Vulkan/Metal/DX, bgfx expects SPIRV wrapped in binary format
    const bool isOpenGL = (rendererType == bgfx::RendererType::OpenGL || 
                           rendererType == bgfx::RendererType::OpenGLES);
    
    if (isOpenGL) {
        // For OpenGL: Just copy GLSL source directly
        const uint32_t sourceSize = static_cast<uint32_t>(source.size());
        const bgfx::Memory* mem = bgfx::copy(source.c_str(), sourceSize + 1);  // +1 for null terminator
        
        bgfx::ShaderHandle handle = bgfx::createShader(mem);
        if (!bgfx::isValid(handle) && logger_) {
            logger_->Error("bgfx::createShader failed for " + label + 
                          " (renderer=" + RendererTypeName(rendererType) + ")");
        }
        return handle;
    }
    

    // Use PipelineCompilerService to compile shader via bgfx_tools
    std::string tempInputPath = "/tmp/" + label + (isVertex ? ".vert.glsl" : ".frag.glsl");
    std::string tempOutputPath = "/tmp/" + label + (isVertex ? ".vert.bin" : ".frag.bin");
    // Write source to tempInputPath
    {
        std::ofstream ofs(tempInputPath);
        ofs << source;
    }
    std::vector<std::string> args;
    // Add any required args for bgfx_tools/shaderc here (e.g., profile, macros)
    bool success = pipelineCompiler_ && pipelineCompiler_->Compile(tempInputPath, tempOutputPath, args);
    if (!success) {
        std::string error = pipelineCompiler_ ? pipelineCompiler_->GetLastError().value_or("") : "No compiler service";
        if (logger_) {
            logger_->Error("PipelineCompilerService failed: " + label + "\n" + error);
        }
        throw std::runtime_error("PipelineCompilerService failed: " + label + "\n" + error);
    }
    // Read compiled binary
    std::ifstream ifs(tempOutputPath, std::ios::binary | std::ios::ate);
    if (!ifs) {
        if (logger_) {
            logger_->Error("Failed to read compiled shader: " + tempOutputPath);
        }
        throw std::runtime_error("Failed to read compiled shader: " + tempOutputPath);
    }
    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!ifs.read(buffer.data(), size)) {
        if (logger_) {
            logger_->Error("Failed to read compiled shader data: " + tempOutputPath);
        }
        throw std::runtime_error("Failed to read compiled shader data: " + tempOutputPath);
    }
    const bgfx::Memory* mem = bgfx::copy(buffer.data(), static_cast<uint32_t>(size));
    bgfx::ShaderHandle handle = bgfx::createShader(mem);
    if (!bgfx::isValid(handle) && logger_) {
        logger_->Error("bgfx::createShader failed for " + label + " (renderer=" + RendererTypeName(rendererType) + ", binSize=" + std::to_string(size) + ")");
    }
    return handle;
}

bgfx::TextureHandle BgfxGraphicsBackend::LoadTextureFromFile(const std::string& path,
                                                             uint64_t samplerFlags) const {
    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "LoadTextureFromFile", "path=" + path);
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels || width <= 0 || height <= 0) {
        if (logger_) {
            logger_->Error("BgfxGraphicsBackend::LoadTextureFromFile: failed to load " + path +
                           " reason=" + (stbi_failure_reason() ? stbi_failure_reason() : "unknown"));
        }
        if (pixels) {
            stbi_image_free(pixels);
        }
        return BGFX_INVALID_HANDLE;
    }

    const uint32_t size = static_cast<uint32_t>(width * height * 4);
    const bgfx::Memory* mem = bgfx::copy(pixels, size);
    stbi_image_free(pixels);

    bgfx::TextureHandle handle = bgfx::createTexture2D(
        static_cast<uint16_t>(width),
        static_cast<uint16_t>(height),
        false,
        1,
        bgfx::TextureFormat::RGBA8,
        samplerFlags,
        mem);

    if (!bgfx::isValid(handle) && logger_) {
        logger_->Error("BgfxGraphicsBackend::LoadTextureFromFile: createTexture2D failed for " + path);
    }

    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "LoadTextureFromFile",
                       "path=" + path +
                           ", width=" + std::to_string(width) +
                           ", height=" + std::to_string(height));
    }

    return handle;
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
    if (!shaderPaths.textures.empty()) {
        const uint64_t samplerFlags = DefaultSamplerFlags();
        uint8_t stage = 0;
        const bgfx::Caps* caps = bgfx::getCaps();
        uint8_t maxStages = 0;
        if (caps) {
            maxStages = static_cast<uint8_t>(std::min<uint32_t>(caps->limits.maxTextureSamplers, 255u));
        }
        if (logger_) {
            logger_->Trace("BgfxGraphicsBackend", "CreatePipeline",
                           "maxTextureSamplers=" + std::to_string(maxStages));
        }
        if (maxStages == 0) {
            if (logger_) {
                logger_->Warn("BgfxGraphicsBackend::CreatePipeline: maxTextureSamplers unavailable for " +
                              shaderKey);
            }
        } else {
            for (const auto& texture : shaderPaths.textures) {
                if (stage >= maxStages) {
                    if (logger_) {
                        logger_->Warn("BgfxGraphicsBackend::CreatePipeline: texture limit reached for " +
                                      shaderKey);
                    }
                    break;
                }
                if (texture.uniformName.empty() || texture.path.empty()) {
                    continue;
                }
                PipelineEntry::TextureBinding binding{};
                binding.stage = stage++;
                binding.uniformName = texture.uniformName;
                binding.sourcePath = texture.path;
                binding.sampler = bgfx::createUniform(binding.uniformName.c_str(),
                                                      bgfx::UniformType::Sampler);
                binding.texture = LoadTextureFromFile(binding.sourcePath, samplerFlags);
                if (!bgfx::isValid(binding.texture)) {
                    binding.texture = CreateSolidTexture(0xff00ffff, samplerFlags);
                }
                if (logger_) {
                    logger_->Trace("BgfxGraphicsBackend", "CreatePipeline",
                                   "shaderKey=" + shaderKey +
                                       ", textureUniform=" + binding.uniformName +
                                       ", texturePath=" + binding.sourcePath +
                                       ", stage=" + std::to_string(binding.stage));
                }
                entry->textures.push_back(std::move(binding));
            }
        }
    }
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
    for (const auto& binding : it->second->textures) {
        if (bgfx::isValid(binding.texture)) {
            bgfx::destroy(binding.texture);
        }
        if (bgfx::isValid(binding.sampler)) {
            bgfx::destroy(binding.sampler);
        }
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

    if (logger_) {
        logger_->Trace("BgfxGraphicsBackend", "Draw",
                       "vertexOffset=" + std::to_string(vertexOffset) +
                       ", indexOffset=" + std::to_string(indexOffset) +
                       ", indexCount=" + std::to_string(indexCount) +
                       ", totalVertices=" + std::to_string(vb->vertexCount));
    }

    // When using indexed rendering with a vertex offset, bgfx expects:
    // - setVertexBuffer: (handle, startVertex=0, numVertices=all)
    // - setIndexBuffer: (handle, firstIndex, numIndices)
    // The indices in the index buffer are already adjusted to reference the correct vertices
    // in the combined vertex buffer, so we should NOT apply vertexOffset again here.
    // Using the full vertex buffer ensures all vertex data is accessible.
    
    // NOTE: Do NOT call bgfx::setTransform() when using MaterialX shaders with explicit uniforms.
    // MaterialX shaders receive transformation matrices via explicit uniforms (u_worldMatrix,
    // u_worldViewProjectionMatrix, etc.) set in ApplyMaterialXUniforms(). Calling setTransform()
    // causes conflicts, especially with Vulkan, resulting in garbage/rainbow artifacts.
    ApplyMaterialXUniforms(modelMatrix);
    for (const auto& binding : pipelineIt->second->textures) {
        if (bgfx::isValid(binding.sampler) && bgfx::isValid(binding.texture)) {
            bgfx::setTexture(binding.stage, binding.sampler, binding.texture);
        }
    }
    bgfx::setVertexBuffer(0, vb->handle);
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
        for (const auto& binding : entry->textures) {
            if (bgfx::isValid(binding.texture)) {
                bgfx::destroy(binding.texture);
            }
            if (bgfx::isValid(binding.sampler)) {
                bgfx::destroy(binding.sampler);
            }
        }
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

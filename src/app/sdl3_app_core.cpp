#include "app/audio_player.hpp"
#include "app/sdl3_app.hpp"
#include "app/trace.hpp"

#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

#ifdef _WIN32
#    include <windows.h>
#endif

namespace sdl3cpp::app {

std::vector<char> ReadFile(const std::string& path) {
    TRACE_FUNCTION();
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("failed to open file: " + path);
    }
    size_t size = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    return buffer;
}

namespace {

#ifdef _WIN32
std::string FormatWin32Error(DWORD errorCode) {
    if (errorCode == ERROR_SUCCESS) {
        return "ERROR_SUCCESS";
    }
    LPSTR buffer = nullptr;
    DWORD length = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        errorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<LPSTR>(&buffer),
        0,
        nullptr);
    std::string message;
    if (length > 0 && buffer) {
        message.assign(buffer, length);
        while (!message.empty() && (message.back() == '\r' || message.back() == '\n')) {
            message.pop_back();
        }
        LocalFree(buffer);
    } else {
        message = "Unknown Windows error";
    }
    return message;
}
#endif

std::string BuildSdlErrorMessage(const char* context) {
    std::ostringstream oss;
    oss << context;
    const char* sdlError = SDL_GetError();
    if (sdlError && *sdlError != '\0') {
        oss << ": " << sdlError;
    } else {
        oss << ": (SDL_GetError returned an empty string)";
    }
#ifdef _WIN32
    DWORD win32Error = ::GetLastError();
    if (win32Error != ERROR_SUCCESS) {
        oss << " [Win32 error " << win32Error << ": " << FormatWin32Error(win32Error) << "]";
    }
#endif
    return oss.str();
}

void ThrowSdlErrorIfFailed(bool success, const char* context) {
    if (!success) {
        throw std::runtime_error(BuildSdlErrorMessage(context));
    }
}

} // namespace

Sdl3App::Sdl3App(const std::filesystem::path& scriptPath, bool luaDebug)
    : cubeScript_(scriptPath, luaDebug),
      scriptDirectory_(scriptPath.parent_path()) {
    TRACE_FUNCTION();
    TRACE_VAR(scriptPath);
}

void Sdl3App::Run() {
    TRACE_FUNCTION();
    InitSDL();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Sdl3App::InitSDL() {
    TRACE_FUNCTION();
    TRACE_VAR(kWidth);
    TRACE_VAR(kHeight);
    ThrowSdlErrorIfFailed(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), "SDL_Init failed");
    ThrowSdlErrorIfFailed(SDL_Vulkan_LoadLibrary(nullptr), "SDL_Vulkan_LoadLibrary failed");
    window_ = SDL_CreateWindow("SDL3 Vulkan Demo", kWidth, kHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        throw std::runtime_error(BuildSdlErrorMessage("SDL_CreateWindow failed"));
    }
    TRACE_VAR(window_);
    SDL_StartTextInput(window_);
    try {
        audioPlayer_ = std::make_unique<AudioPlayer>();
        cubeScript_.SetAudioPlayer(audioPlayer_.get());
    } catch (const std::exception& exc) {
        std::cerr << "AudioPlayer: " << exc.what() << '\n';
    }
}

void Sdl3App::InitVulkan() {
    TRACE_FUNCTION();
    CreateInstance();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
    SetupGuiRenderer();
    CreateImageViews();
    CreateRenderPass();
    LoadSceneData();
    CreateGraphicsPipeline();
    CreateFramebuffers();
    CreateCommandPool();
    CreateVertexBuffer();
    CreateIndexBuffer();
    CreateCommandBuffers();
    CreateSyncObjects();
}

void Sdl3App::MainLoop() {
    TRACE_FUNCTION();
    TRACE_VAR(guiHasCommands_);
    bool running = true;
    auto start = std::chrono::steady_clock::now();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                framebufferResized_ = true;
            }
            ProcessGuiEvent(event);
        }

        float mouseX = 0.0f;
        float mouseY = 0.0f;
        SDL_GetMouseState(&mouseX, &mouseY);
        guiInputSnapshot_.mouseX = mouseX;
        guiInputSnapshot_.mouseY = mouseY;
        cubeScript_.UpdateGuiInput(guiInputSnapshot_);
        if (guiHasCommands_ && guiRenderer_) {
            guiCommands_ = cubeScript_.LoadGuiCommands();
            guiRenderer_->Prepare(guiCommands_, swapChainExtent_.width, swapChainExtent_.height);
        }
        guiInputSnapshot_.wheel = 0.0f;
        guiInputSnapshot_.textInput.clear();

        auto now = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float>(now - start).count();
        DrawFrame(time);
    }

    vkDeviceWaitIdle(device_);
}

void Sdl3App::Cleanup() {
    TRACE_FUNCTION();
    CleanupSwapChain();

    vkDestroyBuffer(device_, vertexBuffer_, nullptr);
    vkFreeMemory(device_, vertexBufferMemory_, nullptr);
    vkDestroyBuffer(device_, indexBuffer_, nullptr);
    vkFreeMemory(device_, indexBufferMemory_, nullptr);
    vkDestroySemaphore(device_, renderFinishedSemaphore_, nullptr);
    vkDestroySemaphore(device_, imageAvailableSemaphore_, nullptr);
    vkDestroyFence(device_, inFlightFence_, nullptr);
    vkDestroyCommandPool(device_, commandPool_, nullptr);

    vkDestroyDevice(device_, nullptr);
    vkDestroySurfaceKHR(instance_, surface_, nullptr);
    vkDestroyInstance(instance_, nullptr);
    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }
    SDL_Vulkan_UnloadLibrary();
    SDL_StopTextInput(window_);
    audioPlayer_.reset();
    SDL_Quit();
}

} // namespace sdl3cpp::app

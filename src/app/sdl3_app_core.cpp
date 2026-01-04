#include "app/audio_player.hpp"
#include "app/sdl3_app.hpp"
#include "logging/logger.hpp"
#include "core/platform.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace sdl3cpp::app {

extern std::atomic<bool> g_signalReceived;

std::vector<char> ReadFile(const std::string& path) {
    sdl3cpp::logging::TraceGuard trace;;
    
    // Validate file exists before attempting to open
    if (!std::filesystem::exists(path)) {
        throw std::runtime_error("File not found: " + path + 
            "\n\nPlease ensure the file exists at this location.");
    }
    
    if (!std::filesystem::is_regular_file(path)) {
        throw std::runtime_error("Path is not a regular file: " + path);
    }
    
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + path + 
            "\n\nThe file exists but cannot be opened. Check file permissions.");
    }
    size_t size = static_cast<size_t>(file.tellg());
    if (size == 0) {
        throw std::runtime_error("File is empty: " + path);
    }
    std::vector<char> buffer(size);
    file.seekg(0);
    file.read(buffer.data(), buffer.size());
    if (!file) {
        throw std::runtime_error("Failed to read file contents: " + path);
    }
    return buffer;
}

namespace {

std::string BuildSdlErrorMessage(const char* context) {
    std::ostringstream oss;
    oss << context;
    const char* sdlError = SDL_GetError();
    if (sdlError && *sdlError != '\0') {
        oss << ": " << sdlError;
    } else {
        oss << ": (SDL_GetError returned an empty string)";
    }

    std::string platformError = sdl3cpp::platform::GetPlatformError();
    if (!platformError.empty() && platformError != "No platform error") {
        oss << " [" << platformError << "]";
    }

    return oss.str();
}

void ThrowSdlErrorIfFailed(bool success, const char* context) {
    if (!success) {
        throw std::runtime_error(BuildSdlErrorMessage(context));
    }
}

void ShowErrorDialog(const char* title, const std::string& message) {
    SDL_ShowSimpleMessageBox(
        SDL_MESSAGEBOX_ERROR,
        title,
        message.c_str(),
        nullptr);
}

} // namespace

Sdl3App::Sdl3App(const std::filesystem::path& scriptPath, bool luaDebug)
    : scriptEngine_(scriptPath, luaDebug),
      scriptDirectory_(scriptPath.parent_path()) {
    sdl3cpp::logging::TraceGuard trace;
    auto& logger = sdl3cpp::logging::Logger::GetInstance();
    logger.TraceVariable("scriptPath", scriptPath.string());
}

bool Sdl3App::ShouldStop() {
    return g_signalReceived.load();
}

void Sdl3App::Run() {
    sdl3cpp::logging::TraceGuard trace;;
    InitSDL();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Sdl3App::InitSDL() {
    sdl3cpp::logging::TraceGuard trace;
    auto& logger = sdl3cpp::logging::Logger::GetInstance();
    logger.TraceVariable("kWidth", static_cast<int>(kWidth));
    logger.TraceVariable("kHeight", static_cast<int>(kHeight));
    
    try {
        ThrowSdlErrorIfFailed(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO), "SDL_Init failed");
    } catch (const std::exception& e) {
        ShowErrorDialog("SDL Initialization Failed", 
            std::string("Failed to initialize SDL subsystems.\n\nError: ") + e.what());
        throw;
    }
    
    try {
        ThrowSdlErrorIfFailed(SDL_Vulkan_LoadLibrary(nullptr), "SDL_Vulkan_LoadLibrary failed");
    } catch (const std::exception& e) {
        ShowErrorDialog("Vulkan Library Load Failed", 
            std::string("Failed to load Vulkan library. Make sure Vulkan drivers are installed.\n\nError: ") + e.what());
        throw;
    }
    
    window_ = SDL_CreateWindow("SDL3 Vulkan Demo", kWidth, kHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        std::string errorMsg = BuildSdlErrorMessage("SDL_CreateWindow failed");
        ShowErrorDialog("Window Creation Failed", 
            std::string("Failed to create application window.\n\nError: ") + errorMsg);
        throw std::runtime_error(errorMsg);
    }
    sdl3cpp::logging::Logger::GetInstance().TraceVariable("window_", window_);
    SDL_StartTextInput(window_);
    try {
        audioPlayer_ = std::make_unique<AudioPlayer>();
        scriptEngine_.SetAudioPlayer(audioPlayer_.get());
    } catch (const std::exception& exc) {
        sdl3cpp::logging::Logger::GetInstance().Warn("AudioPlayer initialization failed: " + std::string(exc.what()));
    }
}

void Sdl3App::InitVulkan() {
    sdl3cpp::logging::TraceGuard trace;;
    try {
        CreateInstance();
    } catch (const std::exception& e) {
        ShowErrorDialog("Vulkan Instance Creation Failed", e.what());
        throw;
    }
    
    try {
        CreateSurface();
    } catch (const std::exception& e) {
        ShowErrorDialog("Vulkan Surface Creation Failed", e.what());
        throw;
    }
    
    try {
        PickPhysicalDevice();
    } catch (const std::exception& e) {
        ShowErrorDialog("GPU Selection Failed", e.what());
        throw;
    }
    
    try {
        CreateLogicalDevice();
    } catch (const std::exception& e) {
        ShowErrorDialog("Logical Device Creation Failed", e.what());
        throw;
    }
    
    try {
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
    } catch (const std::exception& e) {
        std::string errorMsg = "Vulkan initialization failed during resource setup:\n\n";
        errorMsg += e.what();
        ShowErrorDialog("Vulkan Resource Setup Failed", errorMsg);
        throw;
    }
}

void Sdl3App::MainLoop() {
    sdl3cpp::logging::TraceGuard trace;;
    sdl3cpp::logging::Logger::GetInstance().TraceVariable("guiHasCommands_", guiHasCommands_);
    bool running = true;
    auto start = std::chrono::steady_clock::now();
    auto lastProgressTime = start;
    
    // Launch timeout: app must complete first frame within this time
    constexpr auto kLaunchTimeout = std::chrono::seconds(5);
    
    while (running) {
        auto now = std::chrono::steady_clock::now();
        
        // Check for launch timeout if first frame hasn't completed
        if (!firstFrameCompleted_) {
            auto elapsed = now - start;
            if (elapsed > kLaunchTimeout) {
                sdl3cpp::logging::Logger::GetInstance().Error("Launch timeout: Application failed to render first frame within " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(kLaunchTimeout).count()) + " seconds. This typically indicates GPU driver issue, window manager problem, or Vulkan swapchain configuration mismatch.");
                throw std::runtime_error("Launch timeout: First frame did not complete");
            }
            
            // Print progress indicator every second during launch
            if (now - lastProgressTime > std::chrono::seconds(1)) {
                auto waitTime = std::chrono::duration_cast<std::chrono::seconds>(elapsed).count();
                sdl3cpp::logging::Logger::GetInstance().Info("Waiting for first frame... (" + std::to_string(waitTime) + "s)");
                lastProgressTime = now;
            }
        }
        
        if (ShouldStop()) {
            running = false;
            break;
        }
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
        scriptEngine_.UpdateGuiInput(guiInputSnapshot_);
        if (guiHasCommands_ && guiRenderer_) {
            guiCommands_ = scriptEngine_.LoadGuiCommands();
            guiRenderer_->Prepare(guiCommands_, swapChainExtent_.width, swapChainExtent_.height);
        }
        guiInputSnapshot_.wheel = 0.0f;
        guiInputSnapshot_.textInput.clear();

        float time = std::chrono::duration<float>(now - start).count();
        DrawFrame(time);
    }

    vkDeviceWaitIdle(device_);
}

void Sdl3App::Cleanup() {
    sdl3cpp::logging::TraceGuard trace;;
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

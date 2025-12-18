#include "app/sdl3_app.hpp"

#include <chrono>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::app {

std::vector<char> ReadFile(const std::string& path) {
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

Sdl3App::Sdl3App(const std::filesystem::path& scriptPath) : cubeScript_(scriptPath) {}

void Sdl3App::Run() {
    InitSDL();
    InitVulkan();
    MainLoop();
    Cleanup();
}

void Sdl3App::InitSDL() {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    }
    SDL_Vulkan_LoadLibrary(nullptr);
    window_ = SDL_CreateWindow("SDL3 Vulkan Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              kWidth, kHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window_) {
        throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
    }
}

void Sdl3App::InitVulkan() {
    CreateInstance();
    CreateSurface();
    PickPhysicalDevice();
    CreateLogicalDevice();
    CreateSwapChain();
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
    bool running = true;
    auto start = std::chrono::steady_clock::now();
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_WINDOW_SIZE_CHANGED) {
                framebufferResized_ = true;
            }
        }

        auto now = std::chrono::steady_clock::now();
        float time = std::chrono::duration<float>(now - start).count();
        DrawFrame(time);
    }

    vkDeviceWaitIdle(device_);
}

void Sdl3App::Cleanup() {
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
    SDL_Quit();
}

} // namespace sdl3cpp::app

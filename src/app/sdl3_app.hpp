#ifndef SDL3CPP_APP_SDL3_APP_HPP
#define SDL3CPP_APP_SDL3_APP_HPP

#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include "app/audio_player.hpp"
#include "core/vertex.hpp"
#include "script/cube_script.hpp"
#include "gui/gui_renderer.hpp"

namespace sdl3cpp::app {
class AudioPlayer;
}

namespace sdl3cpp::app {

namespace script = sdl3cpp::script;

constexpr uint32_t kWidth = 1024;
constexpr uint32_t kHeight = 768;

inline const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

std::vector<char> ReadFile(const std::string& path);

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class Sdl3App {
public:
    explicit Sdl3App(const std::filesystem::path& scriptPath, bool luaDebug = false);
    void Run();

private:
    struct RenderObject {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;
        int computeModelMatrixRef = LUA_REFNIL;
        std::string shaderKey = "default";
    };

    void InitSDL();
    void InitVulkan();
    void MainLoop();
    void CleanupSwapChain();
    void Cleanup();
    void RecreateSwapChain();
    void CreateInstance();
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSwapChain();
    void CreateImageViews();
    void CreateRenderPass();
    VkShaderModule CreateShaderModule(const std::vector<char>& code);
    void CreateGraphicsPipeline();
    void CreateFramebuffers();
    void CreateCommandPool();
    void LoadSceneData();
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateCommandBuffers();
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, float time,
                             const std::array<float, 16>& viewProj);
    void CreateSyncObjects();
    void DrawFrame(float time);
    void SetupGuiRenderer();
    void ProcessGuiEvent(const SDL_Event& event);

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
    bool IsDeviceSuitable(VkPhysicalDevice device);
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);

    SDL_Window* window_ = nullptr;
    VkInstance instance_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphicsQueue_ = VK_NULL_HANDLE;
    VkQueue presentQueue_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain_ = VK_NULL_HANDLE;
    std::vector<VkImage> swapChainImages_;
    VkFormat swapChainImageFormat_;
    VkExtent2D swapChainExtent_;
    std::vector<VkImageView> swapChainImageViews_;
    VkRenderPass renderPass_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    script::CubeScript cubeScript_;
    std::vector<core::Vertex> vertices_;
    std::vector<uint16_t> indices_;
    std::unordered_map<std::string, script::CubeScript::ShaderPaths> shaderPathMap_;
    std::unordered_map<std::string, VkPipeline> graphicsPipelines_;
    std::string defaultShaderKey_;
    VkFence inFlightFence_ = VK_NULL_HANDLE;
    bool framebufferResized_ = false;
    script::GuiInputSnapshot guiInputSnapshot_;
    std::vector<script::CubeScript::GuiCommand> guiCommands_;
    std::unique_ptr<gui::GuiRenderer> guiRenderer_;
    bool guiHasCommands_ = false;
    std::vector<RenderObject> renderObjects_;
    std::filesystem::path scriptDirectory_;
    std::unique_ptr<AudioPlayer> audioPlayer_;
};

} // namespace sdl3cpp::app

#endif // SDL3CPP_APP_SDL3_APP_HPP

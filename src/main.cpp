#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <vulkan/vulkan.h>

#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <limits>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <lua.hpp>

constexpr uint32_t kWidth = 1024;
constexpr uint32_t kHeight = 768;

struct Vec3 {
    float x;
    float y;
    float z;
};

struct Vertex {
    std::array<float, 3> position;
    std::array<float, 3> color;
};

struct PushConstants {
    std::array<float, 16> model;
    std::array<float, 16> viewProj;
};

static_assert(sizeof(PushConstants) == sizeof(float) * 32, "push constant size mismatch");

const std::vector<const char*> kDeviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

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

std::array<float, 16> MultiplyMatrix(const std::array<float, 16>& a, const std::array<float, 16>& b) {
    std::array<float, 16> result{};
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 4; ++col) {
            float sum = 0.0f;
            for (int idx = 0; idx < 4; ++idx) {
                sum += a[idx * 4 + row] * b[col * 4 + idx];
            }
            result[col * 4 + row] = sum;
        }
    }
    return result;
}

std::array<float, 16> IdentityMatrix() {
    return {1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f};
}

Vec3 Normalize(Vec3 v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len == 0.0f) {
        return v;
    }
    return {v.x / len, v.y / len, v.z / len};
}

Vec3 Cross(const Vec3& a, const Vec3& b) {
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

float Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

std::array<float, 16> LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
    Vec3 f = Normalize({center.x - eye.x, center.y - eye.y, center.z - eye.z});
    Vec3 s = Normalize(Cross(f, up));
    Vec3 u = Cross(s, f);

    std::array<float, 16> result = IdentityMatrix();
    result[0] = s.x;
    result[1] = u.x;
    result[2] = -f.x;
    result[4] = s.y;
    result[5] = u.y;
    result[6] = -f.y;
    result[8] = s.z;
    result[9] = u.z;
    result[10] = -f.z;
    result[12] = -Dot(s, eye);
    result[13] = -Dot(u, eye);
    result[14] = Dot(f, eye);
    return result;
}

std::array<float, 16> Perspective(float fovRadians, float aspect, float zNear, float zFar) {
    float tanHalf = std::tan(fovRadians / 2.0f);
    std::array<float, 16> result{};
    result[0] = 1.0f / (aspect * tanHalf);
    result[5] = -1.0f / tanHalf;
    result[10] = zFar / (zNear - zFar);
    result[11] = -1.0f;
    result[14] = (zNear * zFar) / (zNear - zFar);
    return result;
}

class CubeScript {
public:
    explicit CubeScript(const std::filesystem::path& scriptPath);
    ~CubeScript();

    struct ShaderPaths {
        std::string vertex;
        std::string fragment;
    };

    std::vector<Vertex> LoadVertices();
    std::vector<uint16_t> LoadIndices();
    std::array<float, 16> ComputeModelMatrix(float time);
    ShaderPaths LoadShaderPaths();

private:
    static std::array<float, 3> ReadVector3(lua_State* L, int index);
    static std::array<float, 16> ReadMatrix(lua_State* L, int index);
    static std::string LuaErrorMessage(lua_State* L);
    static ShaderPaths DefaultShaderPaths();

    lua_State* L_ = nullptr;
};

CubeScript::CubeScript(const std::filesystem::path& scriptPath) : L_(luaL_newstate()) {
    if (!L_) {
        throw std::runtime_error("Failed to create Lua state");
    }
    luaL_openlibs(L_);
    if (luaL_dofile(L_, scriptPath.string().c_str()) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        lua_close(L_);
        L_ = nullptr;
        throw std::runtime_error("Failed to load Lua script: " + message);
    }
}

CubeScript::~CubeScript() {
    if (L_) {
        lua_close(L_);
    }
}

std::vector<Vertex> CubeScript::LoadVertices() {
    lua_getglobal(L_, "get_cube_vertices");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_cube_vertices' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_cube_vertices failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_cube_vertices' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    std::vector<Vertex> vertices;
    vertices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_istable(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("Vertex entry at index " + std::to_string(i) + " is not a table");
        }

        int vertexIndex = lua_gettop(L_);
        Vertex vertex{};

        lua_getfield(L_, vertexIndex, "position");
        vertex.position = ReadVector3(L_, -1);
        lua_pop(L_, 1);

        lua_getfield(L_, vertexIndex, "color");
        vertex.color = ReadVector3(L_, -1);
        lua_pop(L_, 1);

        vertices.push_back(vertex);
        lua_pop(L_, 1);
    }

    lua_pop(L_, 1);
    return vertices;
}

std::vector<uint16_t> CubeScript::LoadIndices() {
    lua_getglobal(L_, "get_cube_indices");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'get_cube_indices' is missing");
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_cube_indices failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_cube_indices' did not return a table");
    }

    size_t count = lua_rawlen(L_, -1);
    std::vector<uint16_t> indices;
    indices.reserve(count);

    for (size_t i = 1; i <= count; ++i) {
        lua_rawgeti(L_, -1, static_cast<int>(i));
        if (!lua_isinteger(L_, -1)) {
            lua_pop(L_, 1);
            throw std::runtime_error("Index entry at position " + std::to_string(i) + " is not an integer");
        }
        lua_Integer value = lua_tointeger(L_, -1);
        lua_pop(L_, 1);
        if (value < 1) {
            throw std::runtime_error("Index values must be 1 or greater");
        }
        indices.push_back(static_cast<uint16_t>(value - 1));
    }

    lua_pop(L_, 1);
    return indices;
}

std::array<float, 16> CubeScript::ComputeModelMatrix(float time) {
    lua_getglobal(L_, "compute_model_matrix");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("Lua function 'compute_model_matrix' is missing");
    }
    lua_pushnumber(L_, time);
    if (lua_pcall(L_, 1, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua compute_model_matrix failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'compute_model_matrix' did not return a table");
    }

    std::array<float, 16> matrix = ReadMatrix(L_, -1);
    lua_pop(L_, 1);
    return matrix;
}

CubeScript::ShaderPaths CubeScript::LoadShaderPaths() {
    lua_getglobal(L_, "get_shader_paths");
    if (!lua_isfunction(L_, -1)) {
        lua_pop(L_, 1);
        return DefaultShaderPaths();
    }
    if (lua_pcall(L_, 0, 1, 0) != LUA_OK) {
        std::string message = LuaErrorMessage(L_);
        lua_pop(L_, 1);
        throw std::runtime_error("Lua get_shader_paths failed: " + message);
    }
    if (!lua_istable(L_, -1)) {
        lua_pop(L_, 1);
        throw std::runtime_error("'get_shader_paths' did not return a table");
    }

    ShaderPaths paths;
    lua_getfield(L_, -1, "vertex");
    if (!lua_isstring(L_, -1)) {
        lua_pop(L_, 2);
        throw std::runtime_error("Shader path 'vertex' must be a string");
    }
    paths.vertex = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    lua_getfield(L_, -1, "fragment");
    if (!lua_isstring(L_, -1)) {
        lua_pop(L_, 2);
        throw std::runtime_error("Shader path 'fragment' must be a string");
    }
    paths.fragment = lua_tostring(L_, -1);
    lua_pop(L_, 1);

    lua_pop(L_, 1);
    return paths;
}

std::array<float, 3> CubeScript::ReadVector3(lua_State* L, int index) {
    std::array<float, 3> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 3) {
        throw std::runtime_error("Expected vector with 3 components");
    }
    for (size_t i = 1; i <= 3; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Vector component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::array<float, 16> CubeScript::ReadMatrix(lua_State* L, int index) {
    std::array<float, 16> result{};
    int absIndex = lua_absindex(L, index);
    size_t len = lua_rawlen(L, absIndex);
    if (len != 16) {
        throw std::runtime_error("Expected 4x4 matrix with 16 components");
    }
    for (size_t i = 1; i <= 16; ++i) {
        lua_rawgeti(L, absIndex, static_cast<int>(i));
        if (!lua_isnumber(L, -1)) {
            lua_pop(L, 1);
            throw std::runtime_error("Matrix component is not a number");
        }
        result[i - 1] = static_cast<float>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }
    return result;
}

std::string CubeScript::LuaErrorMessage(lua_State* L) {
    const char* message = lua_tostring(L, -1);
    return message ? message : "unknown lua error";
}

CubeScript::ShaderPaths CubeScript::DefaultShaderPaths() {
    return {"shaders/cube.vert.spv", "shaders/cube.frag.spv"};
}

class VulkanCubeApp {
public:
    explicit VulkanCubeApp(const std::filesystem::path& scriptPath);
    void Run() {
        InitSDL();
        InitVulkan();
        MainLoop();
        Cleanup();
    }

private:
    void InitSDL() {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
        }
        SDL_Vulkan_LoadLibrary(nullptr);
        window_ = SDL_CreateWindow("SDL3 Vulkan Cube Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                  kWidth, kHeight, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
        if (!window_) {
            throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        }
    }

    void InitVulkan() {
        CreateInstance();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandPool();
        LoadCubeData();
        CreateVertexBuffer();
        CreateIndexBuffer();
        CreateCommandBuffers();
        CreateSyncObjects();
    }

    void MainLoop() {
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
            float time =
                std::chrono::duration<float>(now - start).count();
            DrawFrame(time);
        }

        vkDeviceWaitIdle(device_);
    }

    void CleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers_) {
            vkDestroyFramebuffer(device_, framebuffer, nullptr);
        }
        vkFreeCommandBuffers(device_, commandPool_,
                             static_cast<uint32_t>(commandBuffers_.size()), commandBuffers_.data());
        vkDestroyPipeline(device_, graphicsPipeline_, nullptr);
        vkDestroyPipelineLayout(device_, pipelineLayout_, nullptr);
        vkDestroyRenderPass(device_, renderPass_, nullptr);
        for (auto imageView : swapChainImageViews_) {
            vkDestroyImageView(device_, imageView, nullptr);
        }
        vkDestroySwapchainKHR(device_, swapChain_, nullptr);
    }

    void Cleanup() {
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

    void RecreateSwapChain() {
        int width = 0;
        int height = 0;
        while (width == 0 || height == 0) {
            SDL_Vulkan_GetDrawableSize(window_, &width, &height);
            SDL_Event event;
            SDL_WaitEvent(&event);
        }
        vkDeviceWaitIdle(device_);
        CleanupSwapChain();
        CreateSwapChain();
        CreateImageViews();
        CreateRenderPass();
        CreateGraphicsPipeline();
        CreateFramebuffers();
        CreateCommandBuffers();
        framebufferResized_ = false;
    }

    void CreateInstance() {
        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "SDL3 Vulkan Cube";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_2;

        uint32_t extensionCount = 0;
        if (!SDL_Vulkan_GetInstanceExtensions(window_, &extensionCount, nullptr)) {
            throw std::runtime_error("Failed to query Vulkan extensions from SDL");
        }

        std::vector<const char*> extensions(extensionCount);
        if (!SDL_Vulkan_GetInstanceExtensions(window_, &extensionCount, extensions.data())) {
            throw std::runtime_error("Failed to store Vulkan extensions from SDL");
        }

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance");
        }
    }

    void CreateSurface() {
        if (!SDL_Vulkan_CreateSurface(window_, instance_, &surface_)) {
            throw std::runtime_error("Failed to create Vulkan surface");
        }
    }

    void PickPhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("Failed to find GPUs with Vulkan support");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (IsDeviceSuitable(device)) {
                physicalDevice_ = device;
                break;
            }
        }

        if (physicalDevice_ == VK_NULL_HANDLE) {
            throw std::runtime_error("Failed to find a suitable GPU");
        }
    }

    void CreateLogicalDevice() {
        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {*indices.graphicsFamily, *indices.presentFamily};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = &deviceFeatures;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(kDeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = kDeviceExtensions.data();

        if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create logical device");
        }

        vkGetDeviceQueue(device_, *indices.graphicsFamily, 0, &graphicsQueue_);
        vkGetDeviceQueue(device_, *indices.presentFamily, 0, &presentQueue_);
    }

    void CreateSwapChain() {
        SwapChainSupportDetails support = QuerySwapChainSupport(physicalDevice_);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.presentModes);
        VkExtent2D extent = ChooseSwapExtent(support.capabilities);

        uint32_t imageCount = support.capabilities.minImageCount + 1;
        if (support.capabilities.maxImageCount > 0 && imageCount > support.capabilities.maxImageCount) {
            imageCount = support.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface_;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);
        uint32_t queueFamilyIndices[] = {*indices.graphicsFamily, *indices.presentFamily};
        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        if (vkCreateSwapchainKHR(device_, &createInfo, nullptr, &swapChain_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create swap chain");
        }

        vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, nullptr);
        swapChainImages_.resize(imageCount);
        vkGetSwapchainImagesKHR(device_, swapChain_, &imageCount, swapChainImages_.data());

        swapChainImageFormat_ = surfaceFormat.format;
        swapChainExtent_ = extent;
    }

    void CreateImageViews() {
        swapChainImageViews_.resize(swapChainImages_.size());
        for (size_t i = 0; i < swapChainImages_.size(); ++i) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = swapChainImages_[i];
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = swapChainImageFormat_;
            viewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                                   VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = 1;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device_, &viewInfo, nullptr, &swapChainImageViews_[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views");
            }
        }
    }

    void CreateRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat_;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(device_, &renderPassInfo, nullptr, &renderPass_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create render pass");
        }
    }

    VkShaderModule CreateShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device_, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create shader module");
        }
        return shaderModule;
    }

    void CreateGraphicsPipeline() {
        auto shaderPaths = cubeScript_.LoadShaderPaths();
        auto vertShaderCode = ReadFile(shaderPaths.vertex);
        auto fragShaderCode = ReadFile(shaderPaths.fragment);

        VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertStageInfo{};
        vertStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStageInfo.module = vertShaderModule;
        vertStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragStageInfo{};
        fragStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStageInfo.module = fragShaderModule;
        fragStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertStageInfo, fragStageInfo};

        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.vertexAttributeDescriptionCount =
            static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent_.width);
        viewport.height = static_cast<float>(swapChainExtent_.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent_;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;

        VkPushConstantRange pushRange{};
        pushRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
        pushRange.offset = 0;
        pushRange.size = sizeof(PushConstants);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushRange;

        if (vkCreatePipelineLayout(device_, &pipelineLayoutInfo, nullptr, &pipelineLayout_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline layout");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = pipelineLayout_;
        pipelineInfo.renderPass = renderPass_;
        pipelineInfo.subpass = 0;

        if (vkCreateGraphicsPipelines(device_, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                      &graphicsPipeline_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create graphics pipeline");
        }

        vkDestroyShaderModule(device_, fragShaderModule, nullptr);
        vkDestroyShaderModule(device_, vertShaderModule, nullptr);
    }

    void CreateFramebuffers() {
        swapChainFramebuffers_.resize(swapChainImageViews_.size());
        for (size_t i = 0; i < swapChainImageViews_.size(); ++i) {
            VkImageView attachments[] = {swapChainImageViews_[i]};

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass_;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent_.width;
            framebufferInfo.height = swapChainExtent_.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device_, &framebufferInfo, nullptr, &swapChainFramebuffers_[i]) !=
                VK_SUCCESS) {
                throw std::runtime_error("Failed to create framebuffer");
            }
        }
    }

    void CreateCommandPool() {
        QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = *indices.graphicsFamily;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create command pool");
        }
    }

    void LoadCubeData() {
        vertices_ = cubeScript_.LoadVertices();
        indices_ = cubeScript_.LoadIndices();
        if (vertices_.empty() || indices_.empty()) {
            throw std::runtime_error("Lua script did not provide cube geometry");
        }
    }

    void CreateVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer_,
                     vertexBufferMemory_);

        void* data;
        vkMapMemory(device_, vertexBufferMemory_, 0, bufferSize, 0, &data);
        std::memcpy(data, vertices_.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device_, vertexBufferMemory_);
    }

    void CreateIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();
        CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexBuffer_,
                     indexBufferMemory_);

        void* data;
        vkMapMemory(device_, indexBufferMemory_, 0, bufferSize, 0, &data);
        std::memcpy(data, indices_.data(), static_cast<size_t>(bufferSize));
        vkUnmapMemory(device_, indexBufferMemory_);
    }

    void CreateCommandBuffers() {
        commandBuffers_.resize(swapChainFramebuffers_.size());

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool_;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers_.size());

        if (vkAllocateCommandBuffers(device_, &allocInfo, commandBuffers_.data()) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate command buffers");
        }
    }

    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, const PushConstants& pushConstants) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass_;
        renderPassInfo.framebuffer = swapChainFramebuffers_[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent_;

        VkClearValue clearColor = {{{0.1f, 0.1f, 0.15f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline_);

        VkBuffer vertexBuffers[] = {vertexBuffer_};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer_, 0, VK_INDEX_TYPE_UINT16);
        vkCmdPushConstants(commandBuffer, pipelineLayout_, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants),
                           &pushConstants);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices_.size()), 1, 0, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
        vkEndCommandBuffer(commandBuffer);
    }

    void CreateSyncObjects() {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        if (vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &imageAvailableSemaphore_) != VK_SUCCESS ||
            vkCreateSemaphore(device_, &semaphoreInfo, nullptr, &renderFinishedSemaphore_) != VK_SUCCESS ||
            vkCreateFence(device_, &fenceInfo, nullptr, &inFlightFence_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create semaphores");
        }
    }

    void DrawFrame(float time) {
        vkWaitForFences(device_, 1, &inFlightFence_, VK_TRUE, std::numeric_limits<uint64_t>::max());
        vkResetFences(device_, 1, &inFlightFence_);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(device_, swapChain_, std::numeric_limits<uint64_t>::max(),
                                                imageAvailableSemaphore_, VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
            RecreateSwapChain();
            return;
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to acquire swap chain image");
        }

        PushConstants pushConstants{};
        pushConstants.model = cubeScript_.ComputeModelMatrix(time);
        auto view = LookAt({2.0f, 2.0f, 2.5f}, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f});
        auto projection = Perspective(0.78f, static_cast<float>(swapChainExtent_.width) /
                                                   static_cast<float>(swapChainExtent_.height),
                                      0.1f, 10.0f);
        pushConstants.viewProj = MultiplyMatrix(projection, view);

        vkResetCommandBuffer(commandBuffers_[imageIndex], 0);
        RecordCommandBuffer(commandBuffers_[imageIndex], imageIndex, pushConstants);

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphore_};
        VkSemaphore signalSemaphores[] = {renderFinishedSemaphore_};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers_[imageIndex];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue_, 1, &submitInfo, inFlightFence_) != VK_SUCCESS) {
            throw std::runtime_error("Failed to submit draw command buffer");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapChain_;
        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue_, &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized_) {
            RecreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to present swap chain image");
        }
    }

    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }
            ++i;
        }

        return indices;
    }

    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);
        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);
        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount,
                                                      details.presentModes.data());
        }

        return details;
    }

    bool IsDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = FindQueueFamilies(device);
        bool extensionsSupported = CheckDeviceExtensionSupport(device);
        bool swapChainAdequate = false;
        if (extensionsSupported) {
            auto details = QuerySwapChainSupport(device);
            swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
                availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }
        return availableFormats[0];
    }

    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        int width, height;
        SDL_Vulkan_GetDrawableSize(window_, &width, &height);
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(std::clamp(width, static_cast<int>(capabilities.minImageExtent.width),
                                              static_cast<int>(capabilities.maxImageExtent.width))),
            static_cast<uint32_t>(std::clamp(height, static_cast<int>(capabilities.minImageExtent.height),
                                              static_cast<int>(capabilities.maxImageExtent.height)))
        };
        return actualExtent;
    }

    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                      VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex =
            FindMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate buffer memory");
        }

        vkBindBufferMemory(device_, buffer, bufferMemory, 0);
    }

    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice_, &memProperties);
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
            if ((typeFilter & (1 << i)) &&
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        throw std::runtime_error("Failed to find suitable memory type");
    }

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
    VkPipeline graphicsPipeline_ = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> swapChainFramebuffers_;
    VkCommandPool commandPool_ = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers_;
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;
    VkSemaphore imageAvailableSemaphore_ = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore_ = VK_NULL_HANDLE;
    CubeScript cubeScript_;
    std::vector<Vertex> vertices_;
    std::vector<uint16_t> indices_;
    VkFence inFlightFence_ = VK_NULL_HANDLE;
    bool framebufferResized_ = false;
};

VulkanCubeApp::VulkanCubeApp(const std::filesystem::path& scriptPath)
    : cubeScript_(scriptPath) {}

std::filesystem::path FindScriptPath(const char* argv0) {
    std::filesystem::path executable;
    if (argv0 && *argv0 != '\0') {
        executable = std::filesystem::path(argv0);
        if (executable.is_relative()) {
            executable = std::filesystem::current_path() / executable;
        }
    } else {
        executable = std::filesystem::current_path();
    }
    executable = std::filesystem::weakly_canonical(executable);
    std::filesystem::path scriptPath = executable.parent_path() / "scripts" / "cube_logic.lua";
    if (!std::filesystem::exists(scriptPath)) {
        throw std::runtime_error("Could not find Lua script at " + scriptPath.string());
    }
    return scriptPath;
}

int main(int argc, char** argv) {
    try {
        auto scriptPath = FindScriptPath(argc > 0 ? argv[0] : nullptr);
        VulkanCubeApp app(scriptPath);
        app.Run();
    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

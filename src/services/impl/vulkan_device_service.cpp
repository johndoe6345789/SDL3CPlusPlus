#include "vulkan_device_service.hpp"
#include "../../logging/logger.hpp"
#include <SDL3/SDL_vulkan.h>
#include <iostream>
#include <set>
#include <stdexcept>
#include <cstring>

namespace sdl3cpp::services::impl {

VulkanDeviceService::~VulkanDeviceService() {
    if (device_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void VulkanDeviceService::Initialize(const std::vector<const char*>& deviceExtensions,
                                    bool enableValidationLayers) {
    logging::TraceGuard trace;

    deviceExtensions_ = deviceExtensions;
    validationLayersEnabled_ = enableValidationLayers;

    // Get required extensions from SDL
    uint32_t extensionCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensions) {
        throw std::runtime_error("Failed to query Vulkan extensions from SDL");
    }

    std::vector<const char*> requiredExtensions(extensions, extensions + extensionCount);

    CreateInstance(requiredExtensions);
    PickPhysicalDevice();
}

void VulkanDeviceService::CreateSurface(SDL_Window* window) {
    logging::TraceGuard trace;

    if (!window) {
        throw std::invalid_argument("Window cannot be null");
    }

    CreateSurfaceInternal(window);
}

void VulkanDeviceService::CreateInstance(const std::vector<const char*>& requiredExtensions) {
    logging::TraceGuard trace;

    // Check Vulkan availability
    uint32_t apiVersion = 0;
    VkResult enumResult = vkEnumerateInstanceVersion(&apiVersion);
    if (enumResult != VK_SUCCESS) {
        std::string errorMsg = "Vulkan is not available on this system.\n\n";
        errorMsg += "Please install Vulkan drivers:\n";
        errorMsg += "- Ubuntu/Debian: sudo apt install vulkan-tools libvulkan1\n";
        errorMsg += "- Fedora: sudo dnf install vulkan-tools vulkan-loader\n";
        errorMsg += "- Arch: sudo pacman -S vulkan-tools vulkan-icd-loader\n";
        throw std::runtime_error(errorMsg);
    }

    uint32_t major = VK_API_VERSION_MAJOR(apiVersion);
    uint32_t minor = VK_API_VERSION_MINOR(apiVersion);
    uint32_t patch = VK_API_VERSION_PATCH(apiVersion);
    std::cout << "Vulkan Version: " << major << "." << minor << "." << patch << "\n";

    if (apiVersion < VK_API_VERSION_1_2) {
        throw std::runtime_error("Vulkan 1.2 or higher required");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SDL3 Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    // Enable validation layers if requested
    std::vector<const char*> layerList;
    if (validationLayersEnabled_) {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        const char* validationLayer = "VK_LAYER_KHRONOS_validation";
        for (const auto& layer : availableLayers) {
            if (strcmp(layer.layerName, validationLayer) == 0) {
                layerList.push_back(validationLayer);
                std::cout << "Validation layer enabled: " << validationLayer << "\n";
                break;
            }
        }
    }

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(layerList.size());
    createInfo.ppEnabledLayerNames = layerList.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create Vulkan instance");
    }
}

void VulkanDeviceService::CreateSurface(SDL_Window* window) {
    logging::TraceGuard trace;

    if (!SDL_Vulkan_CreateSurface(window, instance_, nullptr, &surface_)) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
}

void VulkanDeviceService::PickPhysicalDevice() {
    logging::TraceGuard trace;

    uint32_t deviceCount = 0;
    VkResult enumResult = vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (enumResult != VK_SUCCESS || deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    std::cout << "\n=== GPU Detection ===\n";
    for (size_t i = 0; i < devices.size(); ++i) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);

        std::cout << "GPU " << i << ": " << props.deviceName << "\n";

        if (IsDeviceSuitable(devices[i])) {
            physicalDevice_ = devices[i];
            std::cout << "  -> SELECTED\n";
            break;
        } else {
            std::cout << "  -> UNSUITABLE\n";
        }
    }
    std::cout << "==================\n\n";

    if (physicalDevice_ == VK_NULL_HANDLE) {
        throw std::runtime_error("Failed to find a suitable GPU");
    }
}

bool VulkanDeviceService::IsDeviceSuitable(VkPhysicalDevice device) const {
    QueueFamilyIndices indices = FindQueueFamilies(device);
    bool extensionsSupported = CheckDeviceExtensionSupport(device);

    bool swapChainAdequate = false;
    if (extensionsSupported) {
        // Check swapchain support
        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_, &capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount, nullptr);

        swapChainAdequate = formatCount > 0 && presentModeCount > 0;
    }

    return indices.IsComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices VulkanDeviceService::FindQueueFamilies(VkPhysicalDevice device) const {
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }

        if (indices.IsComplete()) {
            break;
        }
    }

    return indices;
}

bool VulkanDeviceService::CheckDeviceExtensionSupport(VkPhysicalDevice device) const {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions_.begin(), deviceExtensions_.end());

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void VulkanDeviceService::CreateLogicalDevice() {
    logging::TraceGuard trace;

    QueueFamilyIndices indices = FindQueueFamilies(physicalDevice_);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

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
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions_.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions_.data();

    if (vkCreateDevice(physicalDevice_, &createInfo, nullptr, &device_) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create logical device");
    }

    logging::Logger::GetInstance().Info("Logical device created successfully");
    vkGetDeviceQueue(device_, indices.graphicsFamily, 0, &graphicsQueue_);
    vkGetDeviceQueue(device_, indices.presentFamily, 0, &presentQueue_);
}

void VulkanDeviceService::Shutdown() noexcept {
    if (device_ != VK_NULL_HANDLE) {
        vkDestroyDevice(device_, nullptr);
        device_ = VK_NULL_HANDLE;
    }

    if (surface_ != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
        surface_ = VK_NULL_HANDLE;
    }

    if (instance_ != VK_NULL_HANDLE) {
        vkDestroyInstance(instance_, nullptr);
        instance_ = VK_NULL_HANDLE;
    }
}

void VulkanDeviceService::WaitIdle() {
    if (device_ != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(device_);
    }
}

QueueFamilyIndices VulkanDeviceService::GetQueueFamilies() const {
    return FindQueueFamilies(physicalDevice_);
}

uint32_t VulkanDeviceService::FindMemoryType(uint32_t typeFilter,
                                             VkMemoryPropertyFlags properties) const {
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

}  // namespace sdl3cpp::services::impl

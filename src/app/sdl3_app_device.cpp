#include "app/sdl3_app.hpp"
#include "app/trace.hpp"

#include <set>
#include <stdexcept>
#include <vector>

namespace sdl3cpp::app {

void Sdl3App::CreateInstance() {
    TRACE_FUNCTION();
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "SDL3 Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    uint32_t extensionCount = 0;
    const char* const* extensions = SDL_Vulkan_GetInstanceExtensions(&extensionCount);
    if (!extensions) {
        throw std::runtime_error("Failed to query Vulkan extensions from SDL");
    }

    std::vector<const char*> extensionList(extensions, extensions + extensionCount);
    TRACE_VAR(extensionCount);
    TRACE_VAR(extensionList.size());

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionList.size());
    createInfo.ppEnabledExtensionNames = extensionList.data();

    if (vkCreateInstance(&createInfo, nullptr, &instance_) != VK_SUCCESS) {
        std::string errorMsg = "Failed to create Vulkan instance. This may be due to:\n";
        errorMsg += "- Missing or outdated Vulkan drivers\n";
        errorMsg += "- Incompatible GPU\n";
        errorMsg += "- Missing required Vulkan extensions\n\n";
        errorMsg += "Required extensions (" + std::to_string(extensionList.size()) + "):\n";
        for (const auto* ext : extensionList) {
            errorMsg += "  - ";
            errorMsg += ext;
            errorMsg += "\n";
        }
        throw std::runtime_error(errorMsg);
    }
}

void Sdl3App::CreateSurface() {
    TRACE_FUNCTION();
    if (!SDL_Vulkan_CreateSurface(window_, instance_, nullptr, &surface_)) {
        throw std::runtime_error("Failed to create Vulkan surface");
    }
}

void Sdl3App::PickPhysicalDevice() {
    TRACE_FUNCTION();
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance_, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("Failed to find GPUs with Vulkan support.\n\nPlease ensure:\n- You have a compatible GPU\n- Vulkan drivers are properly installed\n- Your GPU supports Vulkan 1.2 or higher");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance_, &deviceCount, devices.data());

    std::string deviceInfo;
    for (size_t i = 0; i < devices.size(); ++i) {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(devices[i], &props);
        deviceInfo += "\nGPU " + std::to_string(i) + ": " + props.deviceName;
        if (IsDeviceSuitable(devices[i])) {
            physicalDevice_ = devices[i];
            deviceInfo += " [SELECTED]";
            break;
        } else {
            deviceInfo += " [UNSUITABLE]";
        }
    }

    if (physicalDevice_ == VK_NULL_HANDLE) {
        std::string errorMsg = "Failed to find a suitable GPU.\n\n";
        errorMsg += "Found " + std::to_string(deviceCount) + " GPU(s), but none meet the requirements.";
        errorMsg += deviceInfo;
        errorMsg += "\n\nRequired features:\n";
        errorMsg += "- Graphics queue support\n";
        errorMsg += "- Present queue support\n";
        errorMsg += "- Swapchain extension support (VK_KHR_swapchain)\n";
        errorMsg += "- Adequate swapchain formats and present modes\n";
        throw std::runtime_error(errorMsg);
    }
}

void Sdl3App::CreateLogicalDevice() {
    TRACE_FUNCTION();
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

QueueFamilyIndices Sdl3App::FindQueueFamilies(VkPhysicalDevice device) {
    TRACE_FUNCTION();
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

bool Sdl3App::CheckDeviceExtensionSupport(VkPhysicalDevice device) {
    TRACE_FUNCTION();
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(kDeviceExtensions.begin(), kDeviceExtensions.end());
    
    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }
    
    if (!requiredExtensions.empty()) {
        std::cerr << "Missing required device extensions:\n";
        for (const auto& missing : requiredExtensions) {
            std::cerr << "  - " << missing << "\n";
        }
    }

    return requiredExtensions.empty();
}

SwapChainSupportDetails Sdl3App::QuerySwapChainSupport(VkPhysicalDevice device) {
    TRACE_FUNCTION();
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

bool Sdl3App::IsDeviceSuitable(VkPhysicalDevice device) {
    TRACE_FUNCTION();
    QueueFamilyIndices indices = FindQueueFamilies(device);

    bool extensionsSupported = CheckDeviceExtensionSupport(device);
    bool swapChainAdequate = false;
    if (extensionsSupported) {
        auto details = QuerySwapChainSupport(device);
        swapChainAdequate = !details.formats.empty() && !details.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

} // namespace sdl3cpp::app

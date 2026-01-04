#include "app/vulkan_api.hpp"
#include "logging/logger.hpp"

#include <algorithm>
#include <stdexcept>

namespace sdl3cpp::app::vulkan {

namespace {

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            sdl3cpp::logging::Logger::GetInstance().Debug("Found suitable memory type: " + std::to_string(i));
            return i;
        }
    }

    sdl3cpp::logging::Logger::GetInstance().Error("Failed to find suitable memory type");
    throw std::runtime_error("Failed to find suitable memory type");
}

} // namespace

VkExtent2D ChooseSwapExtent(VkSurfaceCapabilitiesKHR capabilities, SDL_Window* window) {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(window, &width, &height);

    return VkExtent2D{
        static_cast<uint32_t>(std::clamp(width, static_cast<int>(capabilities.minImageExtent.width),
                                          static_cast<int>(capabilities.maxImageExtent.width))),
        static_cast<uint32_t>(std::clamp(height, static_cast<int>(capabilities.minImageExtent.height),
                                          static_cast<int>(capabilities.maxImageExtent.height)))
    };
}

void CreateBuffer(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size, VkBufferUsageFlags usage,
                  VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    sdl3cpp::logging::Logger::GetInstance().Debug("Creating buffer with size " + std::to_string(size) + " bytes");
    // Validate buffer size
    if (size == 0) {
        sdl3cpp::logging::Logger::GetInstance().Error("Cannot create buffer with size 0");
        throw std::runtime_error("Cannot create buffer with size 0");
    }
    
    // Check available memory before allocating
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProps);
    
    uint64_t totalAvailable = 0;
    for (uint32_t i = 0; i < memProps.memoryHeapCount; ++i) {
        if (memProps.memoryHeaps[i].flags & VK_MEMORY_HEAP_DEVICE_LOCAL_BIT) {
            totalAvailable += memProps.memoryHeaps[i].size;
        }
    }
    
    if (size > totalAvailable) {
        throw std::runtime_error("Requested buffer size (" + 
            std::to_string(size / (1024 * 1024)) + " MB) exceeds available GPU memory (" +
            std::to_string(totalAvailable / (1024 * 1024)) + " MB)");
    }
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult createResult = vkCreateBuffer(device, &bufferInfo, nullptr, &buffer);
    if (createResult != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer (error code: " + 
            std::to_string(createResult) + ", size: " + 
            std::to_string(size / 1024) + " KB)");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    
    try {
        allocInfo.memoryTypeIndex =
            FindMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties);
    } catch (const std::exception& e) {
        vkDestroyBuffer(device, buffer, nullptr);
        throw std::runtime_error(std::string("Failed to find suitable memory type: ") + e.what());
    }

    VkResult allocResult = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    if (allocResult != VK_SUCCESS) {
        vkDestroyBuffer(device, buffer, nullptr);
        std::string errorMsg = "Failed to allocate buffer memory.\n";
        errorMsg += "Requested: " + std::to_string(memRequirements.size / (1024 * 1024)) + " MB\n";
        errorMsg += "Error code: " + std::to_string(allocResult) + "\n";
        if (allocResult == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
            errorMsg += "\nOut of GPU memory. Try:\n";
            errorMsg += "- Closing other GPU-intensive applications\n";
            errorMsg += "- Reducing window resolution\n";
            errorMsg += "- Upgrading GPU or system memory";
        } else if (allocResult == VK_ERROR_OUT_OF_HOST_MEMORY) {
            errorMsg += "\nOut of system memory. Try:\n";
            errorMsg += "- Closing other applications\n";
            errorMsg += "- Adding more RAM to your system";
        }
        throw std::runtime_error(errorMsg);
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

} // namespace sdl3cpp::app::vulkan

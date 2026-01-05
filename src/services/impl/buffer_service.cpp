#include "buffer_service.hpp"
#include <cstring>
#include <stdexcept>
#include <string>

namespace sdl3cpp::services::impl {

BufferService::BufferService(std::shared_ptr<IVulkanDeviceService> deviceService, std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)), logger_(logger) {
    if (logger_) {
        logger_->Trace("BufferService", "BufferService",
                       "deviceService=" + std::string(deviceService_ ? "set" : "null"));
    }
}

BufferService::~BufferService() {
    if (logger_) {
        logger_->Trace("BufferService", "~BufferService");
    }
    if (vertexBuffer_ != VK_NULL_HANDLE || indexBuffer_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void BufferService::UploadVertexData(const std::vector<core::Vertex>& vertices) {
    logger_->Trace("BufferService", "UploadVertexData",
                   "vertices.size=" + std::to_string(vertices.size()));

    if (vertices.empty()) {
        throw std::runtime_error("Cannot upload vertex data: empty vertex array");
    }

    // Cleanup old buffer if exists
    if (vertexBuffer_ != VK_NULL_HANDLE) {
        auto device = deviceService_->GetDevice();
        vkDestroyBuffer(device, vertexBuffer_, nullptr);
        vkFreeMemory(device, vertexBufferMemory_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
        vertexBufferMemory_ = VK_NULL_HANDLE;
    }

    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    logger_->Info("Uploading vertex buffer: " + std::to_string(vertices.size()) +
                                       " vertices (" + std::to_string(bufferSize / 1024) + " KB)");

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                vertexBuffer_, vertexBufferMemory_);

    auto device = deviceService_->GetDevice();
    void* data;
    vkMapMemory(device, vertexBufferMemory_, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, vertexBufferMemory_);

    vertexCount_ = vertices.size();
}

void BufferService::UploadIndexData(const std::vector<uint16_t>& indices) {
    logger_->Trace("BufferService", "UploadIndexData",
                   "indices.size=" + std::to_string(indices.size()));

    if (indices.empty()) {
        throw std::runtime_error("Cannot upload index data: empty index array");
    }

    // Cleanup old buffer if exists
    if (indexBuffer_ != VK_NULL_HANDLE) {
        auto device = deviceService_->GetDevice();
        vkDestroyBuffer(device, indexBuffer_, nullptr);
        vkFreeMemory(device, indexBufferMemory_, nullptr);
        indexBuffer_ = VK_NULL_HANDLE;
        indexBufferMemory_ = VK_NULL_HANDLE;
    }

    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    logger_->Info("Uploading index buffer: " + std::to_string(indices.size()) +
                                       " indices (" + std::to_string(bufferSize / 1024) + " KB)");

    CreateBuffer(bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                indexBuffer_, indexBufferMemory_);

    auto device = deviceService_->GetDevice();
    void* data;
    vkMapMemory(device, indexBufferMemory_, 0, bufferSize, 0, &data);
    std::memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device, indexBufferMemory_);

    indexCount_ = indices.size();
}

void BufferService::Cleanup() {
    logger_->Trace("BufferService", "Cleanup");
    CleanupBuffers();
}

void BufferService::Shutdown() noexcept {
    logger_->Trace("BufferService", "Shutdown");
    CleanupBuffers();
}

void BufferService::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    logger_->Trace("BufferService", "CreateBuffer",
                   "size=" + std::to_string(size) +
                   ", usage=" + std::to_string(static_cast<uint32_t>(usage)) +
                   ", properties=" + std::to_string(static_cast<uint32_t>(properties)) +
                   ", bufferIsNull=" + std::string(buffer == VK_NULL_HANDLE ? "true" : "false") +
                   ", bufferMemoryIsNull=" + std::string(bufferMemory == VK_NULL_HANDLE ? "true" : "false"));

    auto device = deviceService_->GetDevice();
    auto physicalDevice = deviceService_->GetPhysicalDevice();

    if (logger_) {
        logger_->Debug("Creating buffer with size " + std::to_string(size) + " bytes");
    }

    if (size == 0) {
        if (logger_) {
            logger_->Error("Cannot create buffer with size 0");
        }
        throw std::runtime_error("Cannot create buffer with size 0");
    }

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

    bool foundType = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProps.memoryTypes[i].propertyFlags & properties) == properties) {
            allocInfo.memoryTypeIndex = i;
            foundType = true;
            if (logger_) {
                logger_->Debug("Found suitable memory type: " + std::to_string(i));
            }
            break;
        }
    }

    if (!foundType) {
        vkDestroyBuffer(device, buffer, nullptr);
        if (logger_) {
            logger_->Error("Failed to find suitable memory type");
        }
        throw std::runtime_error("Failed to find suitable memory type");
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

void BufferService::CleanupBuffers() {
    logger_->Trace("BufferService", "CleanupBuffers");

    auto device = deviceService_->GetDevice();

    if (vertexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, vertexBuffer_, nullptr);
        vertexBuffer_ = VK_NULL_HANDLE;
    }

    if (vertexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, vertexBufferMemory_, nullptr);
        vertexBufferMemory_ = VK_NULL_HANDLE;
    }

    if (indexBuffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(device, indexBuffer_, nullptr);
        indexBuffer_ = VK_NULL_HANDLE;
    }

    if (indexBufferMemory_ != VK_NULL_HANDLE) {
        vkFreeMemory(device, indexBufferMemory_, nullptr);
        indexBufferMemory_ = VK_NULL_HANDLE;
    }

    vertexCount_ = 0;
    indexCount_ = 0;
}

}  // namespace sdl3cpp::services::impl

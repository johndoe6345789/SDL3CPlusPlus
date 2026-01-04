#include "buffer_service.hpp"
#include "../../core/vulkan_utils.hpp"
#include <cstring>
#include <stdexcept>

namespace sdl3cpp::services::impl {

BufferService::BufferService(std::shared_ptr<IVulkanDeviceService> deviceService, std::shared_ptr<ILogger> logger)
    : deviceService_(std::move(deviceService)), logger_(logger) {}

BufferService::~BufferService() {
    if (vertexBuffer_ != VK_NULL_HANDLE || indexBuffer_ != VK_NULL_HANDLE) {
        Shutdown();
    }
}

void BufferService::UploadVertexData(const std::vector<core::Vertex>& vertices) {
    logger_->TraceFunction(__func__);

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
    logger_->TraceFunction(__func__);

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
    CleanupBuffers();
}

void BufferService::Shutdown() noexcept {
    logger_->TraceFunction(__func__);
    CleanupBuffers();
}

void BufferService::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                 VkMemoryPropertyFlags properties,
                                 VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    logger_->TraceFunction(__func__);

    auto device = deviceService_->GetDevice();
    auto physicalDevice = deviceService_->GetPhysicalDevice();

    vulkan::utils::CreateBuffer(device, physicalDevice, size, usage, properties, buffer, bufferMemory);
}

void BufferService::CleanupBuffers() {
    logger_->TraceFunction(__func__);

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

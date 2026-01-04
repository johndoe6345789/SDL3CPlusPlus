#pragma once

#include "../interfaces/i_buffer_service.hpp"
#include "../interfaces/i_vulkan_device_service.hpp"
#include "../../di/lifecycle.hpp"
#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Buffer service implementation.
 *
 * Small, focused service (~150 lines) for Vulkan buffer management.
 * Handles vertex and index buffer creation and memory management.
 */
class BufferService : public IBufferService,
                      public di::IShutdownable {
public:
    explicit BufferService(std::shared_ptr<IVulkanDeviceService> deviceService);
    ~BufferService() override;

    // IBufferService interface
    void UploadVertexData(const std::vector<core::Vertex>& vertices) override;
    void UploadIndexData(const std::vector<uint16_t>& indices) override;
    void Cleanup() override;

    VkBuffer GetVertexBuffer() const override { return vertexBuffer_; }
    VkBuffer GetIndexBuffer() const override { return indexBuffer_; }
    size_t GetVertexCount() const override { return vertexCount_; }
    size_t GetIndexCount() const override { return indexCount_; }

    // Public buffer creation utility
    void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                     VkBuffer& buffer, VkDeviceMemory& bufferMemory) override;

    // IShutdownable interface
    void Shutdown() noexcept override;

private:
    std::shared_ptr<IVulkanDeviceService> deviceService_;

    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory_ = VK_NULL_HANDLE;

    size_t vertexCount_ = 0;
    size_t indexCount_ = 0;

    // Helper methods
    void CleanupBuffers();
};

}  // namespace sdl3cpp::services::impl

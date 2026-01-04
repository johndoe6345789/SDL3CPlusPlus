#pragma once

#include "../../core/vertex.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {

/**
 * @brief Buffer service interface.
 *
 * Manages Vulkan buffers for vertex and index data.
 * Small, focused service (~150 lines) for buffer management.
 */
class IBufferService {
public:
    virtual ~IBufferService() = default;

    /**
     * @brief Upload vertex data to GPU.
     *
     * @param vertices Vector of vertex data
     * @throws std::runtime_error if upload fails
     */
    virtual void UploadVertexData(const std::vector<core::Vertex>& vertices) = 0;

    /**
     * @brief Upload index data to GPU.
     *
     * @param indices Vector of index data
     * @throws std::runtime_error if upload fails
     */
    virtual void UploadIndexData(const std::vector<uint16_t>& indices) = 0;

    /**
     * @brief Cleanup all buffers.
     */
    virtual void Cleanup() = 0;

    /**
     * @brief Get the vertex buffer.
     *
     * @return VkBuffer handle
     */
    virtual VkBuffer GetVertexBuffer() const = 0;

    /**
     * @brief Get the index buffer.
     *
     * @return VkBuffer handle
     */
    virtual VkBuffer GetIndexBuffer() const = 0;

    /**
     * @brief Get the number of vertices.
     *
     * @return Vertex count
     */
    virtual size_t GetVertexCount() const = 0;

    /**
     * @brief Get the number of indices.
     *
     * @return Index count
     */
    virtual size_t GetIndexCount() const = 0;
};

}  // namespace sdl3cpp::services

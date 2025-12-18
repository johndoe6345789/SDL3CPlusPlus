#include "app/sdl3_app.hpp"

#include <cstring>
#include <stdexcept>

#include "app/vulkan_api.hpp"

namespace sdl3cpp::app {

void Sdl3App::LoadSceneData() {
    shaderPathMap_ = cubeScript_.LoadShaderPathsMap();
    if (shaderPathMap_.empty()) {
        throw std::runtime_error("Lua script did not provide shader paths");
    }
    defaultShaderKey_ = shaderPathMap_.count("default") ? "default" : shaderPathMap_.begin()->first;

    auto sceneObjects = cubeScript_.LoadSceneObjects();
    if (sceneObjects.empty()) {
        throw std::runtime_error("Lua script did not provide any scene objects");
    }

    vertices_.clear();
    indices_.clear();
    renderObjects_.clear();

    size_t vertexOffset = 0;
    size_t indexOffset = 0;
    for (const auto& sceneObject : sceneObjects) {
        RenderObject renderObject{};
        renderObject.vertexOffset = static_cast<int32_t>(vertexOffset);
        renderObject.indexOffset = static_cast<uint32_t>(indexOffset);
        renderObject.indexCount = static_cast<uint32_t>(sceneObject.indices.size());
        renderObject.computeModelMatrixRef = sceneObject.computeModelMatrixRef;
        renderObject.shaderKey = sceneObject.shaderKey;
        if (shaderPathMap_.find(renderObject.shaderKey) == shaderPathMap_.end()) {
            renderObject.shaderKey = defaultShaderKey_;
        }
        renderObjects_.push_back(renderObject);

        vertices_.insert(vertices_.end(), sceneObject.vertices.begin(), sceneObject.vertices.end());
        for (uint16_t index : sceneObject.indices) {
            indices_.push_back(static_cast<uint16_t>(index + vertexOffset));
        }

        vertexOffset += sceneObject.vertices.size();
        indexOffset += sceneObject.indices.size();
    }

    if (vertices_.empty() || indices_.empty()) {
        throw std::runtime_error("Aggregated scene geometry is empty");
    }
}

void Sdl3App::CreateVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices_[0]) * vertices_.size();
    vulkan::CreateBuffer(device_, physicalDevice_, bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, vertexBuffer_,
                         vertexBufferMemory_);

    void* data;
    vkMapMemory(device_, vertexBufferMemory_, 0, bufferSize, 0, &data);
    std::memcpy(data, vertices_.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device_, vertexBufferMemory_);
}

void Sdl3App::CreateIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices_[0]) * indices_.size();
    vulkan::CreateBuffer(device_, physicalDevice_, bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, indexBuffer_,
                         indexBufferMemory_);

    void* data;
    vkMapMemory(device_, indexBufferMemory_, 0, bufferSize, 0, &data);
    std::memcpy(data, indices_.data(), static_cast<size_t>(bufferSize));
    vkUnmapMemory(device_, indexBufferMemory_);
}

} // namespace sdl3cpp::app

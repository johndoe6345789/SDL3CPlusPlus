#pragma once

#include "../interfaces/i_graphics_backend.hpp"
#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/kernel/sysmem.h>
#include <memory>
#include <unordered_map>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief GXM implementation of the graphics backend for PS Vita.
 */
class GxmGraphicsBackend : public IGraphicsBackend {
public:
    GxmGraphicsBackend();
    ~GxmGraphicsBackend() override;

    void Initialize(void* window, const GraphicsConfig& config) override;
    void Shutdown() override;

    GraphicsDeviceHandle CreateDevice() override;
    void DestroyDevice(GraphicsDeviceHandle device) override;

    GraphicsPipelineHandle CreatePipeline(GraphicsDeviceHandle device, const std::string& shaderKey, const ShaderPaths& shaderPaths) override;
    void DestroyPipeline(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline) override;

    GraphicsBufferHandle CreateVertexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) override;
    GraphicsBufferHandle CreateIndexBuffer(GraphicsDeviceHandle device, const std::vector<uint8_t>& data) override;
    void DestroyBuffer(GraphicsDeviceHandle device, GraphicsBufferHandle buffer) override;

    bool BeginFrame(GraphicsDeviceHandle device) override;
    bool EndFrame(GraphicsDeviceHandle device) override;

    void SetViewProjection(const std::array<float, 16>& viewProj) override;

    void Draw(GraphicsDeviceHandle device, GraphicsPipelineHandle pipeline,
              GraphicsBufferHandle vertexBuffer, GraphicsBufferHandle indexBuffer,
              uint32_t indexCount, const std::array<float, 16>& modelMatrix) override;

private:
    // GXM-specific members
    SceGxmContext* context_;
    SceGxmRenderTarget* renderTarget_;
    SceGxmShaderPatcher* shaderPatcher_;
    SceGxmColorSurface colorSurface_;
    SceGxmDepthStencilSurface depthStencilSurface_;
    void* displayBufferData_[2];
    SceUID displayBufferUid_[2];
    SceGxmSyncObject* displayBufferSync_[2];
    unsigned int backBufferIndex_;
    unsigned int frontBufferIndex_;
    bool initialized_;
    
    // Shader programs cache
    std::unordered_map<std::string, SceGxmShaderPatcherId> vertexShaderIds_;
    std::unordered_map<std::string, SceGxmShaderPatcherId> fragmentShaderIds_;
    std::unordered_map<std::string, SceGxmVertexProgram*> vertexPrograms_;
    std::unordered_map<std::string, SceGxmFragmentProgram*> fragmentPrograms_;
    
    // Memory management
    void* vdmRingBuffer_;
    void* vertexRingBuffer_;
    void* fragmentRingBuffer_;
    void* fragmentUsseRingBuffer_;
    SceUID vdmRingBufferUid_;
    SceUID vertexRingBufferUid_;
    SceUID fragmentRingBufferUid_;
    SceUID fragmentUsseRingBufferUid_;
    
    // Helper methods
    int createDisplayBuffers();
    void destroyDisplayBuffers();
    SceGxmShaderPatcherId loadShader(const std::string& shaderPath, bool isVertex);
    int createShaderPrograms(const std::string& shaderKey, const ShaderPaths& shaderPaths);
    void destroyShaderPrograms();
};

}  // namespace sdl3cpp::services::impl
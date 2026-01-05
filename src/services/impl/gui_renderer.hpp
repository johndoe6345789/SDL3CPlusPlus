#ifndef SDL3CPP_SERVICES_GUI_RENDERER_HPP
#define SDL3CPP_SERVICES_GUI_RENDERER_HPP

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "services/interfaces/gui_types.hpp"
#include "services/interfaces/i_buffer_service.hpp"

namespace sdl3cpp::services::impl {

struct GuiVertex {
    float x, y, z;
    float r, g, b, a;
};

struct SvgCircle {
    float cx = 0.0f;
    float cy = 0.0f;
    float r = 0.0f;
    GuiColor color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct ParsedSvg {
    float viewWidth = 1.0f;
    float viewHeight = 1.0f;
    std::vector<SvgCircle> circles;
};

class GuiRenderer {
public:
    GuiRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapchainFormat,
                VkRenderPass renderPass, const std::filesystem::path& scriptDirectory,
                std::shared_ptr<IBufferService> bufferService);
    ~GuiRenderer();

    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer& operator=(const GuiRenderer&) = delete;

    void Prepare(const std::vector<GuiCommand>& commands, uint32_t width,
                 uint32_t height);
    void RenderToSwapchain(VkCommandBuffer commandBuffer, VkRenderPass renderPass);
    void Resize(uint32_t width, uint32_t height, VkFormat format);
    bool IsReady() const;

private:
    const ParsedSvg* LoadSvg(const std::string& relativePath);

    void CreatePipeline(VkRenderPass renderPass, VkExtent2D extent);
    void CreateVertexAndIndexBuffers(size_t vertexCount, size_t indexCount);
    void CleanupPipeline();
    void CleanupBuffers();
    void UpdateFormat(VkFormat format);
    void GenerateGuiGeometry(const std::vector<GuiCommand>& commands, uint32_t width, uint32_t height);

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkFormat swapchainFormat_;
    VkRenderPass renderPass_;
    std::filesystem::path scriptDirectory_;

    // Pipeline resources
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout_ = VK_NULL_HANDLE;
    VkShaderModule vertShaderModule_ = VK_NULL_HANDLE;
    VkShaderModule fragShaderModule_ = VK_NULL_HANDLE;

    // Vertex/index buffers
    VkBuffer vertexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory vertexMemory_ = VK_NULL_HANDLE;
    VkBuffer indexBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory indexMemory_ = VK_NULL_HANDLE;

    // Geometry data
    std::vector<GuiVertex> vertices_;
    std::vector<uint32_t> indices_;

    uint32_t viewportWidth_ = 0;
    uint32_t viewportHeight_ = 0;
    std::unordered_map<std::string, ParsedSvg> svgCache_;
    std::shared_ptr<IBufferService> bufferService_;
};

} // namespace sdl3cpp::services::impl

#endif // SDL3CPP_SERVICES_GUI_RENDERER_HPP

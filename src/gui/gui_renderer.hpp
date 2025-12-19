#ifndef SDL3CPP_GUI_GUI_RENDERER_HPP
#define SDL3CPP_GUI_GUI_RENDERER_HPP

#include <filesystem>
#include <memory>
#include <unordered_map>
#include <vector>

#include <vulkan/vulkan.h>

#include "script/cube_script.hpp"

namespace sdl3cpp::gui {

struct SvgCircle {
    float cx = 0.0f;
    float cy = 0.0f;
    float r = 0.0f;
    script::GuiColor color{1.0f, 1.0f, 1.0f, 1.0f};
};

struct ParsedSvg {
    float viewWidth = 1.0f;
    float viewHeight = 1.0f;
    std::vector<SvgCircle> circles;
};

class GuiRenderer {
public:
    GuiRenderer(VkDevice device, VkPhysicalDevice physicalDevice, VkFormat swapchainFormat,
                const std::filesystem::path& scriptDirectory);
    ~GuiRenderer();

    GuiRenderer(const GuiRenderer&) = delete;
    GuiRenderer& operator=(const GuiRenderer&) = delete;

    void Prepare(const std::vector<script::CubeScript::GuiCommand>& commands, uint32_t width,
                 uint32_t height);
    void BlitToSwapchain(VkCommandBuffer commandBuffer, VkImage image);
    void Resize(uint32_t width, uint32_t height, VkFormat format);
    bool IsReady() const;

private:
    struct Canvas;

    const ParsedSvg* LoadSvg(const std::string& relativePath);

    void EnsureCanvas(uint32_t width, uint32_t height);
    void UpdateStagingBuffer();
    void CreateStagingBuffer(size_t size);
    void DestroyStagingBuffer();
    void UpdateFormat(VkFormat format);

    VkDevice device_;
    VkPhysicalDevice physicalDevice_;
    VkFormat swapchainFormat_;
    std::filesystem::path scriptDirectory_;
    VkBuffer stagingBuffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stagingMemory_ = VK_NULL_HANDLE;
    void* stagingMapped_ = nullptr;
    size_t stagingSize_ = 0;
    uint32_t canvasWidth_ = 0;
    uint32_t canvasHeight_ = 0;
    std::unique_ptr<Canvas> canvas_;
    std::unordered_map<std::string, ParsedSvg> svgCache_;
};

} // namespace sdl3cpp::gui

#endif // SDL3CPP_GUI_GUI_RENDERER_HPP

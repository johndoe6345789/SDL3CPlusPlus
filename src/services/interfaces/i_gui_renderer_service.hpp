#pragma once

#include "gui_types.hpp"
#include <filesystem>
#include <vector>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {

class IGuiRendererService {
public:
    virtual ~IGuiRendererService() = default;

    virtual void Initialize(VkDevice device,
                            VkPhysicalDevice physicalDevice,
                            VkFormat format,
                            VkRenderPass renderPass,
                            const std::filesystem::path& resourcePath) = 0;

    virtual void PrepareFrame(const std::vector<GuiCommand>& commands,
                              uint32_t width,
                              uint32_t height) = 0;

    virtual void RenderToSwapchain(VkCommandBuffer commandBuffer, VkImage image) = 0;

    virtual void Resize(uint32_t width, uint32_t height, VkFormat format) = 0;

    virtual void Shutdown() noexcept = 0;

    virtual bool IsReady() const = 0;
};

}  // namespace sdl3cpp::services

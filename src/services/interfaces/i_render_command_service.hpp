#pragma once

#include "graphics_types.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

namespace sdl3cpp::services {

/**
 * @brief Render command service interface.
 *
 * Manages command buffer recording and frame synchronization.
 * Small, focused service (~200 lines) for rendering orchestration.
 */
class IRenderCommandService {
public:
    virtual ~IRenderCommandService() = default;

    /**
     * @brief Cleanup command pools and buffers.
     */
    virtual void Cleanup() = 0;

    /**
     * @brief Begin a new frame.
     *
     * Waits for the previous frame to complete and acquires the next image.
     *
     * @param imageIndex Output parameter for the swapchain image index
     * @return true if successful, false if swapchain needs recreation
     */
    virtual bool BeginFrame(uint32_t& imageIndex) = 0;

    /**
     * @brief Record rendering commands.
     *
     * @param imageIndex Swapchain image index
     * @param commands Render commands to execute
     * @param viewProj View-projection matrix
     */
    virtual void RecordCommands(uint32_t imageIndex,
                               const std::vector<RenderCommand>& commands,
                               const std::array<float, 16>& viewProj) = 0;

    /**
     * @brief End the frame and present.
     *
     * Submits command buffers and presents the image.
     *
     * @param imageIndex Swapchain image index
     * @return true if successful, false if swapchain needs recreation
     */
    virtual bool EndFrame(uint32_t imageIndex) = 0;

    /**
     * @brief Get the current command buffer.
     *
     * @return VkCommandBuffer handle
     */
    virtual VkCommandBuffer GetCurrentCommandBuffer() const = 0;

    /**
     * @brief Get the current frame index.
     *
     * @return Frame index (0 to maxFramesInFlight-1)
     */
    virtual uint32_t GetCurrentFrameIndex() const = 0;

    /**
     * @brief Get the maximum frames in flight.
     *
     * @return Max concurrent frames
     */
    virtual uint32_t GetMaxFramesInFlight() const = 0;

    /**
     * @brief Handle swapchain recreation.
     *
     * Resets command buffers/synchronization and updates GUI renderer state.
     */
    virtual void OnSwapchainRecreated() = 0;
};

}  // namespace sdl3cpp::services

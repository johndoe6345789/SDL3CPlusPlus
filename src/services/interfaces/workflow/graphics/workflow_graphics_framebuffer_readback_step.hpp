#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Copy GPU texture to CPU-accessible transfer buffer and store raw pixel data in context.
 *
 * Plugin ID: graphics.framebuffer.readback
 *
 * Inputs:
 *   - source_texture_key: context key holding the SDL_GPUTexture* to read back (default: "gpu_swapchain_texture")
 *   - width:  readback width  (context key holding uint32_t, or sourced from window)
 *   - height: readback height (context key holding uint32_t, or sourced from window)
 *
 * Outputs:
 *   - output_key: context key where std::vector<uint8_t> pixel data is stored
 *   - output_width:  actual readback width  (uint32_t)
 *   - output_height: actual readback height (uint32_t)
 *   - success: bool
 *
 * This step performs the pure GPU readback operation:
 *   1. Acquires a command buffer
 *   2. Blits the source texture to a staging texture
 *   3. Creates a GPU transfer buffer for download
 *   4. Copies staging texture to transfer buffer via copy pass
 *   5. Waits on fence for GPU completion
 *   6. Maps the transfer buffer and copies pixel data into a std::vector<uint8_t>
 *   7. Cleans up all GPU resources
 */
class WorkflowGraphicsFramebufferReadbackStep final : public IWorkflowStep {
public:
    explicit WorkflowGraphicsFramebufferReadbackStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

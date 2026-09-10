#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Copies the swapchain framebuffer to CPU memory via a GPU readback.
 *
 * Plugin ID: graphics.framebuffer.readback
 *
 * Inputs:
 *   - source_texture_key: context key holding the SDL_GPUTexture* to
 *     validate exists before reading back (default:
 *     "gpu_swapchain_texture"); the pixels themselves always come from the
 *     current swapchain texture, blitted to a same-format staging texture.
 *
 * Outputs:
 *   - output_key: context key where std::vector<uint8_t> pixel data is
 *     stored
 *   - output_width/output_height: actual readback dimensions (uint32_t)
 *   - success: bool
 *
 * See gpu_framebuffer_readback.hpp for the blit + download implementation.
 */
class WorkflowGraphicsFramebufferReadbackStep final : public IWorkflowStep {
public:
    explicit WorkflowGraphicsFramebufferReadbackStep(
        std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step,
                 WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

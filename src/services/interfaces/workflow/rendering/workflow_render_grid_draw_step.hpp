#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Per-frame draw call for the cube grid renderer.
 *
 * Plugin ID: "render.grid.draw"
 *
 * Reads the grid configuration stored by render.grid.setup, acquires a
 * command buffer, begins a render pass, computes per-cube model-view-
 * projection matrices, submits indexed draw calls, and ends the pass.
 *
 * Must be called inside a frame loop. Expects render.grid.setup to
 * have already run once to populate grid.config and gpu_depth_texture.
 *
 * Context Input (populated by render.grid.setup):
 *   - grid.config (nlohmann::json) - Grid layout and rendering params
 *   - grid.camera_key (std::string) - Context key for camera data
 *   - gpu_device (SDL_GPUDevice*)
 *   - sdl_window (SDL_Window*)
 *   - gpu_pipeline (SDL_GPUGraphicsPipeline*)
 *   - gpu_vertex_buffer (SDL_GPUBuffer*)
 *   - gpu_index_buffer (SDL_GPUBuffer*)
 *   - gpu_depth_texture (SDL_GPUTexture*)
 *   - frame.elapsed (double) - Current elapsed time in seconds
 *
 * Context Output:
 *   - grid.draw_calls (uint32_t) - Draw calls submitted this frame
 *   - grid.cubes_drawn (uint32_t) - Cubes rendered this frame
 */
class WorkflowRenderGridDrawStep final : public IWorkflowStep {
public:
    explicit WorkflowRenderGridDrawStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

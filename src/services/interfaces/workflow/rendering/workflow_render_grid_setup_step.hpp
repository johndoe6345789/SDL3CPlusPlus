#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * One-time initialization for the cube grid renderer.
 *
 * Plugin ID: "render.grid.setup"
 *
 * Validates that GPU resources exist in context, reads grid layout
 * parameters from the step definition, creates a depth texture, and
 * stores everything needed by render.grid.draw into the context.
 *
 * Parameters (from step.parameters):
 *   - grid_width: Number of cubes horizontally (default: 11)
 *   - grid_height: Number of cubes vertically (default: 11)
 *   - grid_spacing: Distance between cube centers (default: 3.0)
 *   - grid_start_x: Starting X position (default: -15.0)
 *   - grid_start_y: Starting Y position (default: -15.0)
 *   - rotation_offset_x: Per-cube X rotation offset multiplier (default: 0.21)
 *   - rotation_offset_y: Per-cube Y rotation offset multiplier (default: 0.37)
 *   - background_color_r/g/b: Background clear color (default: 0.18 each)
 *   - num_frames: Number of frames to render (default: 600)
 *
 * Inputs (from step.inputs):
 *   - geometry: Context key for mesh buffers
 *   - program: Context key for shader program
 *   - camera: Context key for view/projection matrices
 *
 * Context Output:
 *   - grid.config (nlohmann::json) - Grid layout + rendering params
 *   - grid.camera_key (std::string) - Context key for camera data
 *   - gpu_depth_texture (SDL_GPUTexture*) - Depth buffer for the grid
 */
class WorkflowRenderGridSetupStep final : public IWorkflowStep {
public:
    explicit WorkflowRenderGridSetupStep(
        std::shared_ptr<ILogger> logger
    );

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

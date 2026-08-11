#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Atomic step: Take pre-compiled vertex+fragment shaders from context,
 *        create vertex layout + graphics pipeline, store in context.
 *
 * Plugin ID: "graphics.gpu.pipeline.create"
 *
 * Parameters:
 *   vertex_shader_key   (string)  Context key for compiled vertex SDL_GPUShader* (default: "vertex_shader")
 *   fragment_shader_key (string)  Context key for compiled fragment SDL_GPUShader* (default: "fragment_shader")
 *   vertex_format       (string)  "position_color" or "position_uv" (default: "position_color")
 *   pipeline_key        (string)  Context key to store the pipeline (default: "gpu_pipeline")
 *   depth_write         (number)  1 = enable depth write (default: 1)
 *   depth_test          (number)  1 = enable depth test (default: 1)
 *   cull_mode           (string)  "back", "front", "none" (default: "back")
 *   depth_bias          (number)  Depth bias constant factor (default: 0.0)
 *   depth_bias_slope    (number)  Depth bias slope factor (default: 0.0)
 *   num_color_targets   (number)  Number of color render targets (default: 1)
 *   depth_format        (string)  "d32_float" or "d24_unorm_s8" (default: "d32_float")
 *   release_shaders     (number)  1 = release shaders after pipeline creation (default: 1)
 *
 * Requires in context:
 *   "gpu_device"              -> SDL_GPUDevice*
 *   "sdl_window"              -> SDL_Window* (only when num_color_targets > 0, for swapchain format)
 *   <vertex_shader_key>       -> SDL_GPUShader*
 *   <fragment_shader_key>     -> SDL_GPUShader*
 *
 * Stores in context:
 *   <pipeline_key> -> SDL_GPUGraphicsPipeline*
 */
class WorkflowGpuPipelineCreateStep final : public IWorkflowStep {
public:
    explicit WorkflowGpuPipelineCreateStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

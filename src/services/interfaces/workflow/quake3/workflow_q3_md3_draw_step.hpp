#pragma once

#include "services/interfaces/i_workflow_step.hpp"
#include "services/interfaces/i_logger.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * Plugin ID: q3.md3.draw
 *
 * Renders all surfaces of a named MD3 model at a world transform.
 * Supports both free-standing (world-space position + yaw) and
 * viewmodel (camera-relative) modes.
 *
 * Parameters:
 *   prefix      (string)  – context namespace from q3.md3.load ("lower", "upper", etc.)
 *   pos_key     (string)  – context key holding position as JSON [x,y,z] (world mode)
 *   yaw_key     (string)  – context key holding yaw float in radians (world mode)
 *   frame_key   (string)  – override frame index from context (optional)
 *   fps         (float)   – animation playback FPS (default 15)
 *   anim_first  (int)     – first frame of animation clip
 *   anim_count  (int)     – frame count (0 = loop all frames)
 *   viewmodel   (bool)    – if true, render camera-relative (weapon viewmodel)
 *   vm_right    (float)   – viewmodel right offset (default 0.35)
 *   vm_down     (float)   – viewmodel down offset (default -0.3)
 *   vm_fwd      (float)   – viewmodel forward offset (default 0.5)
 *
 * Reads from context:
 *   gpu_render_pass, gpu_command_buffer, gpu_pipeline_textured
 *   render.view_matrix, render.proj_matrix, render.camera_pos
 *   render.shadow_vp, render.frag_uniforms, frame.elapsed
 */
class WorkflowQ3Md3DrawStep final : public IWorkflowStep {
public:
    explicit WorkflowQ3Md3DrawStep(std::shared_ptr<ILogger> logger);
    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;
private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/i_logger.hpp"
#include "services/interfaces/i_workflow_step.hpp"

#include <memory>

namespace sdl3cpp::services::impl {

/**
 * @brief Computes and stores camera view/projection matrices
 *
 * Plugin ID: "camera.setup"
 *
 * Inputs:
 *   - camera_distance (float): Z distance from origin (e.g., 35.0)
 *   - camera_fov (float): Field of view in degrees (e.g., 60.0)
 *   - aspect_ratio (float): Width/Height ratio (e.g., 1.778)
 *   - near_plane (float): Near clip plane (default: 0.1)
 *   - far_plane (float): Far clip plane (default: 100.0)
 *
 * Outputs (stored in context):
 *   - output_key (default: "camera_state"): JSON object with:
 *     {
 *       "view": [16 floats - row-major view matrix],
 *       "projection": [16 floats - row-major projection matrix],
 *       "distance": 35.0,
 *       "fov": 60.0,
 *       "aspect_ratio": 1.778
 *     }
 */
class WorkflowCameraSetupStep final : public IWorkflowStep {
public:
    explicit WorkflowCameraSetupStep(std::shared_ptr<ILogger> logger);

    std::string GetPluginId() const override;
    void Execute(const WorkflowStepDefinition& step, WorkflowContext& context) override;

private:
    std::shared_ptr<ILogger> logger_;
};

}  // namespace sdl3cpp::services::impl

#pragma once

#include "services/interfaces/workflow_context.hpp"
#include "services/interfaces/workflow_step_definition.hpp"

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// camera.fps.update's resolved parameters, each with the original
/// defaults.
struct CameraFpsUpdateParams {
    float sensitivity = 0.003f;
    float eyeHeight   = 1.5f;
    float fovDeg      = 75.0f;
    float nearPlane   = 0.1f;
    float farPlane    = 500.0f;
};

CameraFpsUpdateParams ReadCameraFpsUpdateParams(
    const WorkflowStepDefinition& step);

/// Applies this frame's mouse delta to the yaw/pitch persisted in the
/// context (`camera_yaw`/`camera_pitch`), clamping pitch to +-~86 degrees
/// to prevent flipping, and writes the updated values back.
struct YawPitch {
    float yaw;
    float pitch;
};
YawPitch UpdateCameraFpsYawPitch(WorkflowContext& context, float sensitivity);

/// The eye position: the player body's origin (via `physics_player_body`
/// and its Bullet transform) plus `camera_eye_height` (falling back to
/// `eyeHeight`), or just `(0, eyeHeight, 0)` with no player body.
glm::vec3 ComputeCameraFpsEyePosition(const WorkflowContext& context,
                                      float eyeHeight);

/// Builds the `camera.state` JSON (view/projection matrices, position,
/// look direction) that render.cube_grid and friends expect.
nlohmann::json BuildCameraFpsState(const WorkflowContext& context,
                                   const glm::vec3& eyePos,
                                   const YawPitch& yawPitch,
                                   const CameraFpsUpdateParams& params);

}  // namespace sdl3cpp::services::impl

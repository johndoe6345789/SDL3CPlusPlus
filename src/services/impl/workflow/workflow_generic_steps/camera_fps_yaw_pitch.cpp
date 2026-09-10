#include "services/interfaces/workflow/workflow_generic_steps/camera_fps_update_helpers.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

YawPitch UpdateCameraFpsYawPitch(WorkflowContext& context, float sensitivity) {
    const float mouseRelX = context.Get<float>("input_mouse_rel_x", 0.0f);
    const float mouseRelY = context.Get<float>("input_mouse_rel_y", 0.0f);

    float yaw   = context.Get<float>("camera_yaw", 0.0f);
    float pitch = context.Get<float>("camera_pitch", 0.0f);

    yaw -= mouseRelX * sensitivity;
    pitch -= mouseRelY * sensitivity;  // Inverted Y.

    // Clamp pitch to prevent flipping.
    constexpr float maxPitch = 1.5f;  // ~86 degrees.
    pitch                    = std::clamp(pitch, -maxPitch, maxPitch);

    context.Set<float>("camera_yaw", yaw);
    context.Set<float>("camera_pitch", pitch);
    return YawPitch{yaw, pitch};
}

}  // namespace sdl3cpp::services::impl

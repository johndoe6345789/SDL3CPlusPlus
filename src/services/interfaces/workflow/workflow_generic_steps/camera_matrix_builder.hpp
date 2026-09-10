#pragma once

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

/// Parameters for one `camera.setup` call.
struct CameraSetupParams {
    float distance = 35.0f;
    float fov = 60.0f;
    float aspectRatio = 1.777f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
};

/**
 * @brief Computes a look-at/perspective pair and packs them into JSON.
 *
 * The camera looks from (0, 0, -distance) toward the origin. Returns an
 * object with 16-float "view"/"projection" arrays (GLM's column-major
 * layout) plus the resolved distance/fov/aspect_ratio/near_plane/
 * far_plane and "camera_setup_success": true.
 */
nlohmann::json BuildCameraStateJson(const CameraSetupParams& params);

}  // namespace sdl3cpp::services::impl

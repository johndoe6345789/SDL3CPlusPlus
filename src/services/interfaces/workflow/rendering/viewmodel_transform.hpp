#pragma once

#include <glm/glm.hpp>

namespace sdl3cpp::services::rendering {

/**
 * @brief Camera axes in world space, taken from a view matrix.
 *
 * A view matrix maps world to camera, so its rotation rows are the
 * camera's world-space axes. Forward is negated because the camera
 * looks down its own -Z.
 */
struct CameraBasis {
    glm::vec3 right;
    glm::vec3 up;
    glm::vec3 forward;
};

CameraBasis ExtractCameraBasis(const glm::mat4& view);

/**
 * @brief Model matrix for geometry locked to the camera (a viewmodel).
 *
 * @param offset Placement in camera axes: x right, y up, z toward the
 *               viewer, so a negative z pushes the model forward. This
 *               matches how the workflow JSON authors offset_x/y/z.
 * @param rotationDegrees Local rotation applied before the camera
 *                        orientation, in XYZ order.
 *
 * Anything that needs a point on the viewmodel in world space — the
 * lens a spotlight shines from, for instance — must use this same
 * matrix, or the two placements drift apart.
 */
glm::mat4 BuildViewmodelMatrix(const glm::mat4& view,
                               const glm::vec3& cameraPosition,
                               const glm::vec3& offset,
                               const glm::vec3& rotationDegrees,
                               float scale);

}  // namespace sdl3cpp::services::rendering

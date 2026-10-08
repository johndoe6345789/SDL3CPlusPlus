#pragma once

#include "services/interfaces/workflow/geometry/geometry_plane_helpers.hpp"

#include <glm/glm.hpp>

namespace sdl3cpp::services::impl {

/// The three unit axes a box is built along. Identity axes give an
/// axis-aligned box.
struct SwitchbackAxes {
    glm::vec3 across;
    glm::vec3 up;
    glm::vec3 along;
};

/// Adds one quad as two triangles. `shade` goes in the uv.x slot, which the
/// switchback fragment shaders read as a per-face light level. `material`
/// goes in uv.y and picks the surface pattern in the shader.
void AddSwitchbackQuad(GeometryPlaneMesh& mesh, const glm::vec3& a,
                       const glm::vec3& b, const glm::vec3& c,
                       const glm::vec3& d, float shade,
                       float material = 0.f);

/// Adds a closed box around `centre`. `half` is the half size along each
/// axis of `axes`.
void AddSwitchbackBox(GeometryPlaneMesh& mesh, const glm::vec3& centre,
                      const SwitchbackAxes& axes, const glm::vec3& half,
                      float material = 0.f);

}  // namespace sdl3cpp::services::impl

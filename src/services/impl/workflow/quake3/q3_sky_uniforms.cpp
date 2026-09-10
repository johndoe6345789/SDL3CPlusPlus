#include "services/interfaces/workflow/quake3/q3_sky_uniforms.hpp"

#include "services/interfaces/workflow/rendering/draw_map_vertex_uniforms.hpp"

#include <glm/gtc/matrix_transform.hpp>

namespace sdl3cpp::services::impl {
namespace {

/// tim_hell scrolls its clouds at 0.05/0.1 texture units a second. The
/// dome wraps the cloud texture three times, so a turn of this speed
/// drifts them at a comparable rate.
constexpr float kDriftRadiansPerSecond = 0.18f;

/// Quake composites a second cloud layer additively over the first
/// (killsky_2 with GL_ONE GL_ONE). Only one layer is drawn here, so lift
/// the single layer to land near the brightness of the real pair rather
/// than rendering a noticeably darker sky.
constexpr float kSkyOverbright = 1.8f;

}  // namespace

rendering::VertexUniformData BuildSkyVertexUniforms(const glm::mat4& view,
                                                    const glm::mat4& proj,
                                                    float elapsed) {
    glm::mat4 rotationOnly = view;
    rotationOnly[3]        = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    const glm::mat4 drift =
        glm::rotate(glm::mat4(1.0f), elapsed * kDriftRadiansPerSecond,
                    glm::vec3(0.0f, 1.0f, 0.0f));
    return BuildDrawMapVertexUniforms(rotationOnly * drift, proj,
                                      glm::vec3(0.0f), glm::mat4(1.0f));
}

rendering::FragmentUniformData BuildSkyFragmentUniforms() {
    rendering::FragmentUniformData fu = {};
    fu.light_color[3] = 1.0f;  // exposure
    fu.material[2]    = kSkyOverbright;
    return fu;
}

}  // namespace sdl3cpp::services::impl

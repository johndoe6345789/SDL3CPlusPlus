#include "services/interfaces/workflow/quake3/q3_sky_uniforms.hpp"

#include "services/interfaces/workflow/rendering/draw_map_vertex_uniforms.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// Quake composites a second cloud layer additively over the first
/// (killsky_2 with GL_ONE GL_ONE). Only one layer is drawn here, so lift
/// the single layer to land near the brightness of the real pair rather
/// than rendering a noticeably darker sky.
constexpr float kSkyOverbright = 1.8f;

}  // namespace

rendering::VertexUniformData BuildSkyVertexUniforms(const glm::mat4& view,
                                                    const glm::mat4& proj) {
    // Drop the view's translation so the dome stays centred on the
    // camera: the sky must not get closer as the player walks toward it.
    // The clouds themselves move by scrolling their uvs, the way Quake
    // does it — turning the dome instead swept them around the zenith.
    glm::mat4 rotationOnly = view;
    rotationOnly[3]        = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    return BuildDrawMapVertexUniforms(rotationOnly, proj, glm::vec3(0.0f),
                                      glm::mat4(1.0f));
}

rendering::FragmentUniformData BuildSkyFragmentUniforms() {
    rendering::FragmentUniformData fu = {};
    fu.light_color[3] = 1.0f;  // exposure
    fu.material[2]    = kSkyOverbright;
    return fu;
}

}  // namespace sdl3cpp::services::impl

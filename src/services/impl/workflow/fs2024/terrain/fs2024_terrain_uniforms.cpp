#include "services/interfaces/workflow/fs2024/terrain/fs2024_terrain_uniforms.hpp"

#include "services/interfaces/workflow/fs2024/fs2024_step_params.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

namespace sdl3cpp::services::impl {
namespace {

glm::vec4 Vec4(const float values[4]) {
    return {values[0], values[1], values[2], values[3]};
}

}  // namespace

Fs2024TerrainVertexUniforms BuildFs2024TerrainVertexUniforms(
    const WorkflowContext& context) {
    const auto view =
        context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const auto proj =
        context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    const auto eye =
        context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.f));
    return {proj * view, glm::vec4(eye, 1.f)};
}

Fs2024TerrainFragmentUniforms BuildFs2024TerrainFragmentUniforms(
    const WorkflowStepDefinition& step, const WorkflowContext& context) {
    Fs2024TerrainFragmentUniforms uniforms;
    const auto light = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    uniforms.sunDir = glm::vec4(glm::vec3(Vec4(light.light_dir)), 0.f);
    uniforms.sunColour = Vec4(light.light_color);
    uniforms.ambient = Vec4(light.ambient);

    const auto horizon = context.Get<glm::vec3>(
        "gta5.sky.horizon", glm::vec3(uniforms.fog));
    uniforms.fog = glm::vec4(
        horizon, Fs2024NumberOr(step, "fog_density", uniforms.fog.w));
    // runway/runwayAxis are per-tile (an airport tile's own runway, if
    // it has one) and are filled in by the draw step, not read here.
    return uniforms;
}

}  // namespace sdl3cpp::services::impl

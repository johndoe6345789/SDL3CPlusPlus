#include "services/interfaces/workflow/bl4/bl4_model_uniforms.hpp"

#include "services/interfaces/workflow/rendering/rendering_types.hpp"

namespace sdl3cpp::services::impl {
namespace {

glm::vec4 Vec4(const float values[4]) {
    return {values[0], values[1], values[2], values[3]};
}

}  // namespace

glm::mat4 BuildBl4ViewProj(const WorkflowContext& context) {
    const auto view = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.f));
    const auto proj = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.f));
    return proj * view;
}

Bl4ModelFragmentUniforms BuildBl4ModelFragmentUniforms(const WorkflowContext& context) {
    Bl4ModelFragmentUniforms uniforms;
    const auto light = context.Get<rendering::FragmentUniformData>("render.frag_uniforms",
                                                                    rendering::FragmentUniformData{});
    uniforms.sunDir = glm::vec4(glm::vec3(Vec4(light.light_dir)), 0.f);
    uniforms.sunColor = Vec4(light.light_color);
    uniforms.ambient = Vec4(light.ambient);
    return uniforms;
}

}  // namespace sdl3cpp::services::impl

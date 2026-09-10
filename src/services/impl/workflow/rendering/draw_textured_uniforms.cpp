#include "services/interfaces/workflow/rendering/draw_textured_uniforms.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

void BuildDrawTexturedUniforms(const WorkflowContext& context,
                               const DrawTexturedTransform& transform,
                               float roughness, float metallic,
                               rendering::VertexUniformData& vu,
                               rendering::FragmentUniformData& fu) {
    auto view   = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
    auto proj   = context.Get<glm::mat4>("render.proj_matrix", glm::mat4(1.0f));
    auto camPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));
    const glm::mat4 mvp = proj * view * transform.model;
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));

    vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(transform.model),
                sizeof(float) * 16);
    vu.normal[0]     = transform.normal.x;
    vu.normal[1]     = transform.normal.y;
    vu.normal[2]     = transform.normal.z;
    vu.uv_scale[0]   = 1.0f;
    vu.uv_scale[1]   = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = roughness;
    fu.material[1] = metallic;
}

}  // namespace sdl3cpp::services::impl

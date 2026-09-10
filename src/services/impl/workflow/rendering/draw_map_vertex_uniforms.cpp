#include "services/interfaces/workflow/rendering/draw_map_vertex_uniforms.hpp"

#include <glm/gtc/type_ptr.hpp>

#include <cstring>

namespace sdl3cpp::services::impl {

rendering::VertexUniformData BuildDrawMapVertexUniforms(
    const glm::mat4& view, const glm::mat4& proj, const glm::vec3& camPos,
    const glm::mat4& shadowVP) {
    const glm::mat4 model = glm::mat4(1.0f);
    const glm::mat4 mvp   = proj * view * model;

    rendering::VertexUniformData vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[0]     = 0;
    vu.normal[1]     = 1;
    vu.normal[2]     = 0;
    vu.uv_scale[0]   = 1.0f;
    vu.uv_scale[1]   = 1.0f;
    vu.camera_pos[0] = camPos.x;
    vu.camera_pos[1] = camPos.y;
    vu.camera_pos[2] = camPos.z;
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);
    return vu;
}

}  // namespace sdl3cpp::services::impl

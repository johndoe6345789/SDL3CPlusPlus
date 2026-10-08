#include "services/interfaces/workflow/switchback/checkpoint/switchback_arrow_transform.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

void CopyMatrix(float* out, const glm::mat4& matrix) {
    std::memcpy(out, glm::value_ptr(matrix), sizeof(float) * 16);
}

}  // namespace

glm::mat4 BuildSwitchbackArrowModel(const glm::vec3& car,
                                    const glm::vec3& target, float height) {
    const glm::vec2 ahead(target.x - car.x, target.z - car.z);
    // The arrow points down +Z, which a yaw of atan2(x, z) turns toward
    // the target.
    const float yaw = std::atan2(ahead.x, ahead.y);
    const glm::mat4 placed = glm::translate(
        glm::mat4(1.f), car + glm::vec3(0.f, height, 0.f));
    return glm::rotate(placed, yaw, glm::vec3(0.f, 1.f, 0.f));
}

rendering::VertexUniformData BuildSwitchbackArrowUniforms(
    const glm::mat4& model, const glm::mat4& view, const glm::mat4& proj,
    const glm::vec3& cameraPos) {
    rendering::VertexUniformData uniforms{};
    CopyMatrix(uniforms.mvp, proj * view * model);
    CopyMatrix(uniforms.model_mat, model);
    CopyMatrix(uniforms.shadow_vp, glm::mat4(1.f));
    uniforms.normal[1] = 1.f;
    uniforms.uv_scale[0] = 1.f;
    uniforms.uv_scale[1] = 1.f;
    uniforms.camera_pos[0] = cameraPos.x;
    uniforms.camera_pos[1] = cameraPos.y;
    uniforms.camera_pos[2] = cameraPos.z;
    return uniforms;
}

}  // namespace sdl3cpp::services::impl

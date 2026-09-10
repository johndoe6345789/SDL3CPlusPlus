#include "services/interfaces/workflow/rendering/bsp_portal_view_uniforms.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <cmath>
#include <cstring>

namespace sdl3cpp::services::impl {

void BuildPortalViewUniforms(const WorkflowContext& context,
                             const glm::vec3& dest,
                             rendering::VertexUniformData& vu,
                             rendering::FragmentUniformData& fu) {
    const float yaw   = context.Get<float>("camera_yaw", 0.0f);
    const float pitch = context.Get<float>("camera_pitch", 0.0f);
    glm::vec3 front;
    front.x = std::cos(pitch) * (-std::sin(yaw));
    front.y = std::sin(pitch);
    front.z = std::cos(pitch) * (-std::cos(yaw));
    front   = glm::normalize(front);

    const glm::mat4 view =
        glm::lookAt(dest, dest + front, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 proj =
        glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 500.0f);
    const glm::mat4 model = glm::mat4(1.0f);
    const glm::mat4 mvp   = proj * view * model;

    vu = {};
    std::memcpy(vu.mvp, glm::value_ptr(mvp), sizeof(float) * 16);
    std::memcpy(vu.model_mat, glm::value_ptr(model), sizeof(float) * 16);
    vu.normal[1]     = 1.0f;
    vu.uv_scale[0]   = 1.0f;
    vu.uv_scale[1]   = 1.0f;
    vu.camera_pos[0] = dest.x;
    vu.camera_pos[1] = dest.y;
    vu.camera_pos[2] = dest.z;
    auto shadowVP = context.Get<glm::mat4>("render.shadow_vp", glm::mat4(1.0f));
    std::memcpy(vu.shadow_vp, glm::value_ptr(shadowVP), sizeof(float) * 16);

    fu = context.Get<rendering::FragmentUniformData>(
        "render.frag_uniforms", rendering::FragmentUniformData{});
    fu.material[0] = 0.7f;
    fu.material[1] =
        static_cast<float>(context.GetDouble("frame.elapsed", 0.0));
    fu.material[2] = 2.0f;
    fu.material[3] = 0.0f;
}

}  // namespace sdl3cpp::services::impl

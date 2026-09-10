#include "services/interfaces/workflow/quake3/q3_md3_model_matrix.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <nlohmann/json.hpp>

#include <cmath>

namespace sdl3cpp::services::impl {

glm::mat4 BuildMd3ModelMatrix(const WorkflowContext& context,
                              const Md3DrawParams& params,
                              const glm::mat4& view, const glm::vec3& camPos) {
    if (params.viewmodel) {
        const glm::vec3 right(view[0][0], view[1][0], view[2][0]);
        const glm::vec3 up(view[0][1], view[1][1], view[2][1]);
        const glm::vec3 forward(-view[0][2], -view[1][2], -view[2][2]);
        const glm::vec3 pos = camPos + right * params.vmRight +
                              up * params.vmDown + forward * params.vmFwd;

        glm::mat4 orient(1.0f);
        orient[0] = glm::vec4(forward, 0.0f);
        orient[1] = glm::vec4(-right, 0.0f);
        orient[2] = glm::vec4(up, 0.0f);
        return glm::translate(glm::mat4(1.0f), pos) * orient;
    }

    glm::vec3 pos(0.0f);
    if (!params.posKey.empty()) {
        const auto* pv = context.TryGet<nlohmann::json>(params.posKey);
        if (pv && pv->is_array() && pv->size() >= 3) {
            pos = glm::vec3((*pv)[0].get<float>(), (*pv)[1].get<float>(),
                            (*pv)[2].get<float>());
        }
    }
    float yaw = 0.0f;
    if (!params.yawKey.empty()) {
        yaw = context.Get<float>(params.yawKey, 0.0f);
    }
    // Same Z-up remap as the viewmodel: a bare yaw rotation about world Y
    // leaves a Z-up model lying on its side.
    const glm::vec3 f(-std::sin(yaw), 0.0f, -std::cos(yaw));
    const glm::vec3 u(0.0f, 1.0f, 0.0f);
    const glm::vec3 l = glm::cross(u, f);
    glm::mat4 orient(1.0f);
    orient[0] = glm::vec4(f, 0.0f);
    orient[1] = glm::vec4(l, 0.0f);
    orient[2] = glm::vec4(u, 0.0f);
    return glm::translate(glm::mat4(1.0f), pos) * orient;
}

}  // namespace sdl3cpp::services::impl

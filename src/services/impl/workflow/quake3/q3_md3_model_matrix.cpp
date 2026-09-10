#include "services/interfaces/workflow/quake3/q3_md3_model_matrix.hpp"

#include "services/interfaces/workflow/quake3/q3_axes.hpp"

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

        return q3::PlaceModelWithBasis(pos, forward, up);
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
    return q3::PlaceModel(pos, yaw);
}

}  // namespace sdl3cpp::services::impl

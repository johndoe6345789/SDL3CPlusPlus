#include "services/interfaces/workflow/rendering/render_prepare_helpers.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

glm::vec3 PrepareRenderCameraState(WorkflowContext& context) {
    glm::mat4 viewMatrix(1.0f);
    glm::mat4 projMatrix(1.0f);

    const auto* cam_json = context.TryGet<nlohmann::json>("camera.state");
    if (cam_json) {
        if (cam_json->contains("view")) {
            auto v = (*cam_json)["view"].get<std::vector<float>>();
            if (v.size() == 16) viewMatrix = glm::make_mat4(v.data());
        }
        if (cam_json->contains("projection")) {
            auto p = (*cam_json)["projection"].get<std::vector<float>>();
            if (p.size() == 16) projMatrix = glm::make_mat4(p.data());
        }
    }

    const glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix)[3]);

    context.Set<glm::mat4>("render.view_matrix", viewMatrix);
    context.Set<glm::mat4>("render.proj_matrix", projMatrix);
    context.Set<glm::vec3>("render.camera_pos", cameraPos);
    return cameraPos;
}

}  // namespace sdl3cpp::services::impl

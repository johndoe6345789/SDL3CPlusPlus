#include "services/interfaces/workflow/rendering/workflow_spotlight_update_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <cmath>

namespace sdl3cpp::services::impl {

WorkflowSpotlightUpdateStep::WorkflowSpotlightUpdateStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowSpotlightUpdateStep::GetPluginId() const {
    return "spotlight.update";
}

void WorkflowSpotlightUpdateStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    const auto* spot = context.TryGet<nlohmann::json>("spotlight.state");
    if (!spot) return;

    auto fu = context.Get<rendering::FragmentUniformData>("render.frag_uniforms", rendering::FragmentUniformData{});

    // All values from JSON — no hardcoded defaults in C++
    const std::string attach = spot->value("attach", std::string());
    const auto offset = spot->value("offset", std::vector<float>{});
    const glm::vec3 off(offset.size() > 0 ? offset[0] : 0.0f,
                        offset.size() > 1 ? offset[1] : 0.0f,
                        offset.size() > 2 ? offset[2] : 0.0f);

    glm::vec3 spotPos, spotDir;

    if (attach == "camera") {
        const auto viewMatrix = context.Get<glm::mat4>("render.view_matrix", glm::mat4(1.0f));
        const auto cameraPos = context.Get<glm::vec3>("render.camera_pos", glm::vec3(0.0f));

        const glm::vec3 camRight = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
        const glm::vec3 camUp = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
        const glm::vec3 camFwd = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);

        spotPos = cameraPos + camRight * off.x + camUp * off.y + camFwd * (-off.z);

        const float aimDist = spot->value("aim_distance", 0.0f);
        if (aimDist > 0.0f) {
            const glm::vec3 aimTarget = cameraPos + camFwd * aimDist;
            spotDir = glm::normalize(aimTarget - spotPos);
        } else {
            spotDir = camFwd;
        }
    } else {
        const auto p = spot->value("position", std::vector<float>{});
        const auto d = spot->value("direction", std::vector<float>{});
        spotPos = glm::vec3(p.size() > 0 ? p[0] : 0.0f, p.size() > 1 ? p[1] : 0.0f, p.size() > 2 ? p[2] : 0.0f) + off;
        spotDir = glm::normalize(glm::vec3(d.size() > 0 ? d[0] : 0.0f, d.size() > 1 ? d[1] : 0.0f, d.size() > 2 ? d[2] : -1.0f));
    }

    const float innerCone = spot->value("inner_cone", 0.0f);
    const float outerCone = spot->value("outer_cone", 0.0f);
    const auto col = spot->value("color", std::vector<float>{});
    const float intensity = spot->value("intensity", 0.0f);
    const float range = spot->value("range", 0.0f);

    fu.flash_pos[0] = spotPos.x;
    fu.flash_pos[1] = spotPos.y;
    fu.flash_pos[2] = spotPos.z;
    fu.flash_pos[3] = std::cos(glm::radians(innerCone));
    fu.flash_dir[0] = spotDir.x;
    fu.flash_dir[1] = spotDir.y;
    fu.flash_dir[2] = spotDir.z;
    fu.flash_dir[3] = std::cos(glm::radians(outerCone));
    fu.flash_color[0] = (col.size() > 0 ? col[0] : 0.0f) * intensity;
    fu.flash_color[1] = (col.size() > 1 ? col[1] : 0.0f) * intensity;
    fu.flash_color[2] = (col.size() > 2 ? col[2] : 0.0f) * intensity;
    fu.flash_color[3] = range;

    context.Set<rendering::FragmentUniformData>("render.frag_uniforms", fu);
}

}  // namespace sdl3cpp::services::impl

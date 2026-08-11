#include "services/interfaces/workflow/rendering/workflow_render_prepare_step.hpp"
#include "services/interfaces/workflow/rendering/rendering_types.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>
#include <vector>

namespace sdl3cpp::services::impl {

WorkflowRenderPrepareStep::WorkflowRenderPrepareStep(std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowRenderPrepareStep::GetPluginId() const {
    return "render.prepare";
}

void WorkflowRenderPrepareStep::Execute(const WorkflowStepDefinition& step, WorkflowContext& context) {
    // --- Camera state ---
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

    glm::vec3 cameraPos = glm::vec3(glm::inverse(viewMatrix)[3]);

    context.Set<glm::mat4>("render.view_matrix", viewMatrix);
    context.Set<glm::mat4>("render.proj_matrix", projMatrix);
    context.Set<glm::vec3>("render.camera_pos", cameraPos);

    // --- Shadow state ---
    glm::mat4 shadowVP(1.0f);

    const auto* shadow_json = context.TryGet<nlohmann::json>("shadow.state");
    if (shadow_json && shadow_json->contains("light_vp")) {
        auto vp = (*shadow_json)["light_vp"].get<std::vector<float>>();
        if (vp.size() == 16) shadowVP = glm::make_mat4(vp.data());
    }

    context.Set<glm::mat4>("render.shadow_vp", shadowVP);

    // --- Fragment uniforms from lighting ---
    rendering::FragmentUniformData fu = {};

    fu.light_dir[1] = -1.0f;
    fu.light_color[0] = 1.0f;
    fu.light_color[1] = 1.0f;
    fu.light_color[2] = 1.0f;
    fu.light_color[3] = 1.0f;
    fu.ambient[0] = 0.2f;
    fu.ambient[1] = 0.2f;
    fu.ambient[2] = 0.2f;
    fu.material[0] = 0.8f;
    fu.material[1] = 0.0f;

    const auto* lighting = context.TryGet<nlohmann::json>("lighting.directional");
    if (lighting) {
        if (lighting->contains("direction")) {
            auto dir = (*lighting)["direction"].get<std::vector<float>>();
            if (dir.size() >= 3) {
                fu.light_dir[0] = dir[0];
                fu.light_dir[1] = dir[1];
                fu.light_dir[2] = dir[2];
            }
        }
        if (lighting->contains("color")) {
            auto col = (*lighting)["color"].get<std::vector<float>>();
            if (col.size() >= 3) {
                fu.light_color[0] = col[0];
                fu.light_color[1] = col[1];
                fu.light_color[2] = col[2];
            }
        }
        if (lighting->contains("ambient")) {
            auto amb = (*lighting)["ambient"].get<std::vector<float>>();
            if (amb.size() >= 3) {
                fu.ambient[0] = amb[0];
                fu.ambient[1] = amb[1];
                fu.ambient[2] = amb[2];
            }
        }
        fu.light_color[3] = lighting->value("exposure", 1.0f);
    }

    context.Set<rendering::FragmentUniformData>("render.frag_uniforms", fu);

    if (logger_) {
        logger_->Trace("WorkflowRenderPrepareStep", "Execute",
                       "cam=(" + std::to_string(cameraPos.x) + "," +
                       std::to_string(cameraPos.y) + "," +
                       std::to_string(cameraPos.z) + ")",
                       "Render state prepared");
    }
}

}  // namespace sdl3cpp::services::impl

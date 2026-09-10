#include "services/interfaces/workflow/rendering/render_prepare_helpers.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <nlohmann/json.hpp>

#include <vector>

namespace sdl3cpp::services::impl {

void PrepareRenderShadowState(WorkflowContext& context) {
    glm::mat4 shadowVP(1.0f);

    const auto* shadow_json = context.TryGet<nlohmann::json>("shadow.state");
    if (shadow_json && shadow_json->contains("light_vp")) {
        auto vp = (*shadow_json)["light_vp"].get<std::vector<float>>();
        if (vp.size() == 16) shadowVP = glm::make_mat4(vp.data());
    }

    context.Set<glm::mat4>("render.shadow_vp", shadowVP);
}

}  // namespace sdl3cpp::services::impl

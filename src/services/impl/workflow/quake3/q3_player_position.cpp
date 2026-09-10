#include "services/interfaces/workflow/quake3/q3_player_position.hpp"

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {

glm::vec3 ResolveQ3PlayerPosition(WorkflowContext& context) {
    if (const auto* pp = context.TryGet<glm::vec3>("q3.player_pos")) {
        return *pp;
    }
    const auto camState =
        context.Get<nlohmann::json>("camera.state", nlohmann::json::object());
    if (camState.contains("position") && camState["position"].is_array()) {
        const auto& cp = camState["position"];
        if (cp.size() >= 3) {
            return {cp[0].get<float>(), cp[1].get<float>(), cp[2].get<float>()};
        }
    }
    return glm::vec3(0.f);
}

}  // namespace sdl3cpp::services::impl

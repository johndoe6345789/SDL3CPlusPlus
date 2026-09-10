#include "services/interfaces/workflow/rendering/spotlight_pose_world.hpp"

#include <vector>

namespace sdl3cpp::services::impl {

SpotlightPose ComputeWorldSpotlightPose(const nlohmann::json& spot,
                                        const glm::vec3& offset) {
    const auto p = spot.value("position", std::vector<float>{});
    const auto d = spot.value("direction", std::vector<float>{});
    const glm::vec3 pos =
        glm::vec3(p.size() > 0 ? p[0] : 0.0f, p.size() > 1 ? p[1] : 0.0f,
                  p.size() > 2 ? p[2] : 0.0f) +
        offset;
    const glm::vec3 dir = glm::normalize(
        glm::vec3(d.size() > 0 ? d[0] : 0.0f, d.size() > 1 ? d[1] : 0.0f,
                  d.size() > 2 ? d[2] : -1.0f));
    return SpotlightPose{pos, dir};
}

}  // namespace sdl3cpp::services::impl

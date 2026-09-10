#include "services/interfaces/workflow/quake3/q3_pickup_position.hpp"

#include <sstream>
#include <string>

namespace sdl3cpp::services::impl {
namespace {

// Parse "x y z" origin string into glm::vec3; returns false on failure.
bool ParseOrigin(const std::string& origin, glm::vec3& out) {
    std::istringstream ss(origin);
    float x = 0.0f, y = 0.0f, z = 0.0f;
    if (!(ss >> x >> y >> z)) return false;
    out = glm::vec3(x, y, z);
    return true;
}

}  // namespace

bool ReadEntityPosition(const nlohmann::json& ent, glm::vec3& out) {
    if (ent.contains("origin") && ent["origin"].is_string()) {
        return ParseOrigin(ent["origin"].get<std::string>(), out);
    }
    if (ent.contains("position") && ent["position"].is_array()) {
        const auto& p = ent["position"];
        if (p.size() == 3) {
            out = glm::vec3(p[0].get<float>(), p[1].get<float>(),
                            p[2].get<float>());
            return true;
        }
    }
    return false;
}

}  // namespace sdl3cpp::services::impl

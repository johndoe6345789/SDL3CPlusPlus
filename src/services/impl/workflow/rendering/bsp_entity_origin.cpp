#include "services/interfaces/workflow/rendering/bsp_entity_origin.hpp"
#include "services/interfaces/workflow/rendering/bsp_entity_lump_parser.hpp"
#include "services/interfaces/workflow/rendering/bsp_q3_coordinates.hpp"

namespace sdl3cpp::services::impl {

void ApplyEntityOrigin(
    nlohmann::json& ent, const std::map<std::string, std::string>& values,
    const std::string& classname, float scale,
    std::unordered_map<std::string, std::array<float, 3>>& targets,
    nlohmann::json& spawn) {
    auto originIt = values.find("origin");
    if (originIt == values.end()) {
        return;
    }
    float ox = 0, oy = 0, oz = 0;
    if (!ParseBspVec3(originIt->second, ox, oy, oz)) {
        return;
    }
    const auto p    = ConvertQ3Point(ox, oy, oz, scale);
    ent["position"] = PointJson(p);

    auto targetNameIt = values.find("targetname");
    if (targetNameIt != values.end()) {
        targets[targetNameIt->second] = p;
    }

    if (classname == "info_player_deathmatch" && spawn.is_null()) {
        spawn        = {{"x", p[0]}, {"y", p[1] + 1.0f}, {"z", p[2]}};
        auto angleIt = values.find("angle");
        float angle  = 0.0f;
        if (angleIt != values.end()) {
            try {
                angle = std::stof(angleIt->second);
            } catch (...) {
                angle = 0.0f;
            }
        }
        spawn["angle"] = angle;
    }
}

}  // namespace sdl3cpp::services::impl

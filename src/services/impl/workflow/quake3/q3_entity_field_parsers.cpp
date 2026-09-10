#include "services/interfaces/workflow/quake3/q3_entity_field_parsers.hpp"

#include <cstdio>
#include <string>

namespace sdl3cpp::services::impl {

float EntFloat(const nlohmann::json& ent, const char* key, float def) {
    if (!ent.contains(key)) return def;
    const auto& v = ent[key];
    if (v.is_number()) return v.get<float>();
    if (v.is_string()) {
        try {
            return std::stof(v.get<std::string>());
        } catch (...) {}
    }
    return def;
}

glm::vec3 ParseOrigin(const std::string& s, float scale) {
    float x = 0.f, y = 0.f, z = 0.f;
    std::sscanf(s.c_str(), "%f %f %f", &x, &y, &z);
    return glm::vec3(x * scale, y * scale, z * scale);
}

}  // namespace sdl3cpp::services::impl

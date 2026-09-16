#include "services/interfaces/workflow/gta5/audio/gta5_ambience.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 Vec(const nlohmann::json& v) {
    if (!v.is_array() || v.size() < 3) return glm::vec3(0.f);
    return {v[0].get<float>(), v[1].get<float>(), v[2].get<float>()};
}

Gta5AmbientRule Rule(const nlohmann::json& j) {
    Gta5AmbientRule r;
    r.at        = Vec(j.value("pos", nlohmann::json()));
    r.placed    = glm::length(r.at) > 1.f;
    r.inner     = j.value("inner", 0.f);
    r.outer     = std::max(j.value("outer", 50.f), 1.f);
    r.start     = j.value("start", 0);
    r.end       = j.value("end", 1440);
    r.perMinute = j.value("per_minute", 1.f);
    r.files     = j.value("clips", std::vector<std::string>{});
    return r;
}

}  // namespace

Gta5Ambience LoadGta5Ambience(const std::string& dir,
                              const std::shared_ptr<ILogger>& logger) {
    Gta5Ambience out;
    out.dir = dir;
    std::ifstream in(dir + "/ambience.json");
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (!doc.is_object()) {
        if (logger) logger->Info("gta5.sound: no ambience in " + dir);
        return out;
    }
    for (const auto& r : doc.value("rules", nlohmann::json::array())) {
        out.rules.push_back(Rule(r));
    }
    for (const auto& z : doc.value("zones", nlohmann::json::array())) {
        Gta5AmbientZone zone;
        zone.centre = Vec(z.value("pos", nlohmann::json()));
        zone.half   = Vec(z.value("size", nlohmann::json())) * 0.5f;
        zone.angle  = z.value("angle", 0.f);
        zone.rules  = z.value("rules", std::vector<int>{});
        out.zones.push_back(std::move(zone));
    }
    if (logger) {
        logger->Info("gta5.sound: ambience, " +
                     std::to_string(out.zones.size()) + " zones, " +
                     std::to_string(out.rules.size()) + " rules");
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

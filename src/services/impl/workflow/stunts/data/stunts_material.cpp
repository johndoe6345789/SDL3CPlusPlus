#include "services/interfaces/workflow/stunts/data/stunts_material.hpp"

#include <cstdlib>
#include <fstream>

#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 ParseHexColor(const std::string& hex) {
    if (hex.size() != 6) return glm::vec3(0.5f);
    const auto byte = [&](std::size_t at) {
        return static_cast<float>(std::strtol(hex.substr(at, 2).c_str(),
                                              nullptr, 16)) /
               255.f;
    };
    return {byte(0), byte(2), byte(4)};
}

}  // namespace

StuntsMaterialTable LoadStuntsMaterialTable(const std::string& path) {
    StuntsMaterialTable table;
    std::ifstream file(path);
    if (!file) return table;

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::exception&) {
        return table;
    }
    if (const auto def = root.find("default"); def != root.end() &&
        def->is_string()) {
        table.fallback = ParseHexColor(def->get<std::string>());
    }
    const auto colors = root.find("colors");
    if (colors == root.end() || !colors->is_object()) return table;
    for (const auto& entry : colors->items()) {
        const int id = std::atoi(entry.key().c_str());
        if (id < 0 || id > 255 || !entry.value().is_string()) continue;
        table.colors[static_cast<std::uint8_t>(id)] =
            ParseHexColor(entry.value().get<std::string>());
    }
    table.loaded = true;
    return table;
}

glm::vec3 StuntsColorFor(const StuntsMaterialTable& table, std::uint8_t id) {
    const auto it = table.colors.find(id);
    return it != table.colors.end() ? it->second : table.fallback;
}

}  // namespace sdl3cpp::services::impl

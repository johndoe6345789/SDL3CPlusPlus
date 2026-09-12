#include "services/interfaces/workflow/gta5/hud/gta5_map_poi.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <unordered_map>

namespace sdl3cpp::services::impl {
namespace {

Gta5MapPoiCategory ReadCategory(const nlohmann::json& c) {
    Gta5MapPoiCategory category;
    category.label = c.value("label", std::string());
    const std::string letter = c.value("letter", std::string("?"));
    category.letter = letter.empty() ? '?' : letter[0];
    const nlohmann::json rgb = c.value("colour", nlohmann::json::array());
    for (std::size_t i = 0; i < 3 && i < rgb.size(); ++i) {
        if (rgb[i].is_number()) {
            category.colour[i] = static_cast<std::uint8_t>(rgb[i].get<int>());
        }
    }
    return category;
}

}  // namespace

Gta5MapPois LoadGta5MapPois(const std::string& path,
                            const std::shared_ptr<ILogger>& logger) {
    Gta5MapPois out;
    std::ifstream in(path);
    const nlohmann::json doc =
        in ? nlohmann::json::parse(in, nullptr, false) : nlohmann::json();
    if (!doc.is_object()) {
        if (logger) {
            logger->Warn("gta5.map: no points of interest in '" + path + "'");
        }
        return out;
    }
    std::unordered_map<std::string, int> ids;
    for (const auto& c : doc.value("categories", nlohmann::json::array())) {
        if (!c.is_object() || out.categories.size() >= kGta5MapMaxCategories) {
            continue;
        }
        ids[c.value("id", std::string())] =
            static_cast<int>(out.categories.size());
        out.categories.push_back(ReadCategory(c));
    }
    for (const auto& p : doc.value("points", nlohmann::json::array())) {
        if (!p.is_object()) continue;
        const auto found = ids.find(p.value("category", std::string()));
        if (found == ids.end()) continue;
        out.points.push_back({found->second, p.value("x", 0.f),
                              p.value("y", 0.f),
                              p.value("name", std::string())});
    }
    if (logger) {
        logger->Info("gta5.map: " + std::to_string(out.points.size()) +
                     " points of interest in " +
                     std::to_string(out.categories.size()) + " categories");
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

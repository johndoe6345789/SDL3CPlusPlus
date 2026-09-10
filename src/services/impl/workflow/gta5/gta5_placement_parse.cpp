#include "services/interfaces/workflow/gta5/gta5_placement_parse.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 ReadVec3(const nlohmann::json& node, const glm::vec3& fallback) {
    if (!node.is_array() || node.size() < 3) return fallback;
    return glm::vec3(node.at(0).get<float>(), node.at(1).get<float>(),
                     node.at(2).get<float>());
}

void ReadRotation(const nlohmann::json& entry, Gta5Placement& placement) {
    const auto rotation = entry.find("rotation");
    if (rotation == entry.end() || !rotation->is_array() ||
        rotation->size() < 4) {
        return;
    }
    // Stored [x, y, z, w]; glm::quat takes (w, x, y, z).
    placement.rotation = glm::quat(rotation->at(3).get<float>(),
                                   rotation->at(0).get<float>(),
                                   rotation->at(1).get<float>(),
                                   rotation->at(2).get<float>());
}

}  // namespace

bool ParseGta5Placement(const nlohmann::json& entry,
                        Gta5Placement& outPlacement) {
    outPlacement.archetype = entry.value("archetype", std::string{});
    if (outPlacement.archetype.empty()) return false;

    // The importer writes null for archetypes it had no exported model
    // for. Keep them: the placement is still map data, it simply has
    // nothing to draw, and gta5.tiles.load reports them once.
    const auto model = entry.find("model");
    if (model != entry.end() && model->is_string()) {
        outPlacement.modelPath = model->get<std::string>();
    }

    const auto position = entry.find("position");
    if (position != entry.end()) {
        outPlacement.position = ReadVec3(*position, glm::vec3(0.f));
    }

    ReadRotation(entry, outPlacement);

    const auto scale = entry.find("scale");
    if (scale != entry.end()) {
        outPlacement.scale = ReadVec3(*scale, glm::vec3(1.f));
    }

    outPlacement.lod =
        Gta5LodFromName(entry.value("lod", std::string{"hd"}));
    return true;
}

}  // namespace sdl3cpp::services::impl

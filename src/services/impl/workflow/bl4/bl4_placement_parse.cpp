#include "services/interfaces/workflow/bl4/bl4_placement_parse.hpp"

namespace sdl3cpp::services::impl {
namespace {

glm::vec3 ReadVec3(const nlohmann::json& node, const glm::vec3& fallback) {
    if (!node.is_array() || node.size() < 3) return fallback;
    return glm::vec3(node.at(0).get<float>(), node.at(1).get<float>(),
                     node.at(2).get<float>());
}

void ReadRotation(const nlohmann::json& entry, Bl4Placement& placement) {
    const auto rotation = entry.find("rotation");
    if (rotation == entry.end() || !rotation->is_array() || rotation->size() < 4) return;
    // Stored [x, y, z, w]; glm::quat takes (w, x, y, z).
    placement.rotation = glm::quat(rotation->at(3).get<float>(), rotation->at(0).get<float>(),
                                   rotation->at(1).get<float>(), rotation->at(2).get<float>());
}

}  // namespace

bool ParseBl4Placement(const nlohmann::json& entry, Bl4Placement& outPlacement) {
    outPlacement.archetype = entry.value("archetype", std::string{});
    if (outPlacement.archetype.empty()) return false;

    // bl4x writes no "model" for a mesh it couldn't decode; keep the
    // placement (still map data) with an empty modelPath, so
    // bl4.tiles.load can report it instead of silently dropping it.
    const auto model = entry.find("model");
    if (model != entry.end() && model->is_string()) {
        outPlacement.modelPath = model->get<std::string>();
    }

    const auto position = entry.find("position");
    if (position != entry.end()) outPlacement.position = ReadVec3(*position, glm::vec3(0.f));

    ReadRotation(entry, outPlacement);

    const auto scale = entry.find("scale");
    if (scale != entry.end()) outPlacement.scale = ReadVec3(*scale, glm::vec3(1.f));

    return true;
}

std::vector<Bl4Placement> ParseBl4TilePlacements(const nlohmann::json& doc) {
    std::vector<Bl4Placement> out;
    if (!doc.contains("placements") || !doc["placements"].is_array()) return out;
    for (const auto& entry : doc["placements"]) {
        Bl4Placement placement;
        if (ParseBl4Placement(entry, placement)) out.push_back(std::move(placement));
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

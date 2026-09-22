#include "services/interfaces/workflow/stunts/data/stunts_tile_table.hpp"

#include <fstream>
#include <nlohmann/json.hpp>

namespace sdl3cpp::services::impl {
namespace {

std::uint8_t ParseLinks(const std::string& text) {
    std::uint8_t links = kStuntsLinkNone;
    for (const char edge : text) {
        if (edge == 'N') links |= kStuntsLinkNorth;
        if (edge == 'E') links |= kStuntsLinkEast;
        if (edge == 'S') links |= kStuntsLinkSouth;
        if (edge == 'W') links |= kStuntsLinkWest;
    }
    return links;
}

StuntsTileKind ParseKind(const std::string& text) {
    if (text == "straight") return StuntsTileKind::Straight;
    if (text == "corner") return StuntsTileKind::Corner;
    if (text == "junction") return StuntsTileKind::Junction;
    return StuntsTileKind::None;
}

}  // namespace

StuntsTileTable LoadStuntsTileTable(const std::string& path) {
    StuntsTileTable table;
    std::ifstream file(path);
    if (!file) return table;

    nlohmann::json root;
    try {
        file >> root;
    } catch (const nlohmann::json::exception&) {
        return table;
    }
    const auto tiles = root.find("tiles");
    if (tiles == root.end() || !tiles->is_object()) return table;

    for (const auto& entry : tiles->items()) {
        const int id = std::atoi(entry.key().c_str());
        if (id <= 0 || id > 255) continue;
        StuntsTile tile;
        tile.kind = ParseKind(entry.value().value("kind", std::string()));
        tile.links = ParseLinks(entry.value().value("links", std::string()));
        if (tile.kind == StuntsTileKind::None) continue;
        table.tiles[static_cast<std::size_t>(id)] = tile;
        ++table.known;
    }
    table.loaded = table.known > 0;
    return table;
}

std::uint8_t StuntsOppositeLink(std::uint8_t link) {
    switch (link) {
        case kStuntsLinkNorth: return kStuntsLinkSouth;
        case kStuntsLinkSouth: return kStuntsLinkNorth;
        case kStuntsLinkEast: return kStuntsLinkWest;
        case kStuntsLinkWest: return kStuntsLinkEast;
        default: return kStuntsLinkNone;
    }
}

}  // namespace sdl3cpp::services::impl

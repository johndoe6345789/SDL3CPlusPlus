#include "services/interfaces/workflow/gta5/gta5_streaming_config_load.hpp"

#include "services/interfaces/workflow/gta5/gta5_json_file.hpp"

namespace sdl3cpp::services::impl {
namespace {

void ReadSections(const nlohmann::json& root,
                  Gta5StreamingConfig& streaming) {
    const auto residency = root.find("residency");
    if (residency != root.end() && residency->is_object()) {
        streaming.loadRadiusTiles = residency->value(
            "load_radius_tiles", streaming.loadRadiusTiles);
        streaming.evictRadiusTiles = residency->value(
            "evict_radius_tiles", streaming.evictRadiusTiles);
    }

    const auto budget = root.find("budget");
    if (budget != root.end() && budget->is_object()) {
        streaming.maxResidentTiles =
            budget->value("max_resident_tiles", streaming.maxResidentTiles);
        streaming.maxSpawnsPerFrame = budget->value(
            "max_spawns_per_frame", streaming.maxSpawnsPerFrame);
    }

    const auto prefetch = root.find("prefetch");
    if (prefetch != root.end() && prefetch->is_object()) {
        streaming.prefetchEnabled =
            prefetch->value("enabled", streaming.prefetchEnabled);
        streaming.velocityLeadSeconds = prefetch->value(
            "velocity_lead_seconds", streaming.velocityLeadSeconds);
    }
    const auto vantage = root.find("vantage");
    if (vantage != root.end() && vantage->is_object()) {
        auto& s = streaming;
        s.vantageBaseMetres =
            vantage->value("base_metres", s.vantageBaseMetres);
        s.vantageMetresPerTile =
            vantage->value("metres_per_tile", s.vantageMetresPerTile);
        s.vantageMaxTiles =
            vantage->value("max_extra_tiles", s.vantageMaxTiles);
    }
}

/// An evict radius inside the load radius makes tiles load and unload on
/// alternate frames forever. Clamp rather than honour it.
void ClampRadii(Gta5StreamingConfig& streaming,
                const std::shared_ptr<ILogger>& logger) {
    if (streaming.evictRadiusTiles > streaming.loadRadiusTiles) return;

    if (logger) {
        logger->Warn("gta5 streaming config: evict_radius_tiles (" +
                     std::to_string(streaming.evictRadiusTiles) +
                     ") must exceed load_radius_tiles (" +
                     std::to_string(streaming.loadRadiusTiles) +
                     "); raising it to avoid tile thrashing");
    }
    streaming.evictRadiusTiles = streaming.loadRadiusTiles + 1;
}

}  // namespace

bool LoadGta5StreamingConfig(const std::string& path,
                             Gta5StreamingConfig& streaming,
                             const std::shared_ptr<ILogger>& logger) {
    nlohmann::json root;
    if (!ReadGta5JsonFile(path, root, logger, "gta5 streaming config")) {
        streaming.loaded = true;
        return false;
    }

    ReadSections(root, streaming);
    ClampRadii(streaming, logger);
    streaming.loaded = true;
    return true;
}

}  // namespace sdl3cpp::services::impl

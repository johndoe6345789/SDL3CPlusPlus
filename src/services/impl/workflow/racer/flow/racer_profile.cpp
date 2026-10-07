#include "services/interfaces/workflow/racer/flow/racer_flow_rules.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

/// Copies a JSON array into `out`, clamping each value; missing or
/// malformed entries keep their defaults.
template <typename T, std::size_t N>
void ReadArray(const nlohmann::json& doc, const char* key,
               std::array<T, N>& out, T lo, T hi) {
    const auto values = doc.value(key, nlohmann::json::array());
    for (std::size_t i = 0; i < N && i < values.size(); ++i) {
        if (values[i].is_number()) {
            out[i] = std::clamp(values[i].get<T>(), lo, hi);
        }
    }
}

}  // namespace

RacerProfile LoadRacerProfile(const std::filesystem::path& path) {
    RacerProfile p;
    std::ifstream in(path);
    if (!in) return p;
    const auto doc = nlohmann::json::parse(in, nullptr, false);
    if (doc.is_discarded() || !doc.is_object()) return p;
    p.truguts = std::max(0, doc.value("truguts", p.truguts));
    ReadArray(doc, "upgrades", p.upgrades, 0, kRacerUpgradeMax);
    ReadArray(doc, "health", p.health, 0.f, 1.f);
    p.pitDroids = std::clamp(doc.value("pit_droids", 1), 1,
                             kRacerMaxPitDroids);
    p.racesRun = std::max(0, doc.value("races_run", 0));
    p.podiums = std::max(0, doc.value("podiums", 0));
    p.circuitsOpen = std::clamp(doc.value("circuits_open", 1), 1,
                                kRacerCircuits);
    ReadArray(doc, "tracks_open", p.tracksOpen, 1, kRacerCircuitTracks);
    ReadArray(doc, "best", p.best, 0, 12);
    ReadArray(doc, "points", p.points, 0, 1000000);
    p.racers = doc.value("racers", p.racers) | 1u;  // Anakin, always
    p.planetsSeen = doc.value("planets_seen", 0u);
    p.introSeen = doc.value("intro_seen", false);
    return p;
}

bool SaveRacerProfile(const RacerProfile& p,
                      const std::filesystem::path& path) {
    nlohmann::json doc;
    doc["truguts"] = p.truguts;
    doc["upgrades"] = p.upgrades;
    doc["health"] = p.health;
    doc["pit_droids"] = p.pitDroids;
    doc["races_run"] = p.racesRun;
    doc["podiums"] = p.podiums;
    doc["circuits_open"] = p.circuitsOpen;
    doc["tracks_open"] = p.tracksOpen;
    doc["best"] = p.best;
    doc["points"] = p.points;
    doc["racers"] = p.racers;
    doc["planets_seen"] = p.planetsSeen;
    doc["intro_seen"] = p.introSeen;
    std::ofstream out(path);
    if (!out) return false;
    out << doc.dump(1) << '\n';
    return static_cast<bool>(out);
}

}  // namespace sdl3cpp::services::impl

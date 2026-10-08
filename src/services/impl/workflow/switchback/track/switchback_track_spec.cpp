#include "services/interfaces/workflow/switchback/track/switchback_track_spec.hpp"

#include <nlohmann/json.hpp>

#include <fstream>

namespace sdl3cpp::services::impl {
namespace {

void ReadMountain(const nlohmann::json& doc, SwitchbackMountainSpec& out) {
    out.baseRadiusM = doc.value("baseRadius", out.baseRadiusM);
    out.peakHeightM = doc.value("peakHeight", out.peakHeightM);
    out.roughness = doc.value("roughness", out.roughness);
    out.seed = doc.value("seed", out.seed);
}

void ReadRoad(const nlohmann::json& doc, SwitchbackRoadSpec& out) {
    out.widthM = doc.value("width", out.widthM);
    out.startAngleDegrees =
        doc.value("startAngleDegrees", out.startAngleDegrees);
    out.turns = doc.value("turns", out.turns);
    out.startHeightM = doc.value("startHeight", out.startHeightM);
    out.endHeightM = doc.value("endHeight", out.endHeightM);
    out.maxGradePercent = doc.value("maxGradePercent", out.maxGradePercent);
}

}  // namespace

bool LoadSwitchbackTrackSpec(const std::string& path,
                             SwitchbackTrackSpec& out) {
    std::ifstream file(path);
    if (!file) return false;
    nlohmann::json doc;
    try {
        doc = nlohmann::json::parse(file);
    } catch (const nlohmann::json::exception&) {
        return false;
    }
    out.id = doc.value("id", std::string());
    out.name = doc.value("name", std::string());
    if (doc.contains("terrain")) {
        const nlohmann::json& terrain = doc["terrain"];
        out.gridSize = terrain.value("gridSize", out.gridSize);
        out.heightMaxM = terrain.value("heightMaxM", out.heightMaxM);
        out.noiseAmplitudeM =
            terrain.value("noiseAmplitudeM", out.noiseAmplitudeM);
        out.bankM = terrain.value("bankM", out.bankM);
        out.road.outerRadiusM =
            terrain.value("roadOuterM", out.road.outerRadiusM);
        out.road.innerRadiusM =
            terrain.value("roadInnerM", out.road.innerRadiusM);
        out.road.samples = terrain.value("roadSamples", out.road.samples);
    }
    if (doc.contains("mountain")) ReadMountain(doc["mountain"], out.mountain);
    if (doc.contains("road")) ReadRoad(doc["road"], out.road);
    if (doc.contains("checkpoints")) {
        out.checkpointCount =
            doc["checkpoints"].value("count", out.checkpointCount);
    }
    return true;
}

}  // namespace sdl3cpp::services::impl

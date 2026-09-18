#include "services/interfaces/workflow/fs2024/data/texture/fs2024_pgg_assets.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

float Metres(const nlohmann::json& pair, std::size_t at, float fallback) {
    if (!pair.is_array() || pair.size() <= at) return fallback;
    return pair[at].get<float>();
}

}  // namespace

std::vector<PggAsset> ReadPggAssets(const std::string& jsonPath) {
    std::ifstream in(jsonPath);
    if (!in) throw std::runtime_error("PGG textures '" + jsonPath + "'");
    const nlohmann::json root = nlohmann::json::parse(in);

    std::vector<PggAsset> assets;
    for (const auto& entry : root.value("assets", nlohmann::json::array())) {
        PggAsset asset;
        asset.file = entry.value("file", std::string());
        asset.albedoLayer = entry.value("index_albedo", 0);
        const auto& size = entry["dimensions_in_meters"];
        asset.widthMetres = Metres(size, 0, 1.f);
        asset.heightMetres = Metres(size, 1, 1.f);
        if (entry.contains("offset")) {
            asset.offsetX = Metres(entry["offset"], 0, 0.f);
            asset.offsetY = Metres(entry["offset"], 1, 0.f);
        }
        if (entry.contains("tiling") && entry["tiling"].size() >= 2) {
            asset.tilingX = entry["tiling"][0].get<std::string>();
            asset.tilingY = entry["tiling"][1].get<std::string>();
        }
        assets.push_back(std::move(asset));
    }
    return assets;
}

const PggAsset& FindPggAsset(const std::vector<PggAsset>& assets,
                             const std::string& file) {
    const auto it = std::find_if(assets.begin(), assets.end(),
                                [&](const PggAsset& asset) {
                                    return asset.file == file;
                                });
    if (it == assets.end()) {
        throw std::runtime_error("PGG textures: no asset '" + file + "'");
    }
    return *it;
}

}  // namespace sdl3cpp::fs2024

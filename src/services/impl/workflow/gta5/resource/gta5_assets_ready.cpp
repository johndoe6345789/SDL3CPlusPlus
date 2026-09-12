#include "services/interfaces/workflow/gta5/resource/gta5_assets_index_step.hpp"

#include <algorithm>
#include <chrono>
#include <exception>
#include <future>
#include <string>
#include <thread>

namespace sdl3cpp::services::impl {

bool Gta5AssetsReady(Gta5StreamState& state,
                     const std::shared_ptr<ILogger>& logger) {
    if (state.assets) return true;
    if (!state.assetsPending.valid()) return false;
    if (state.assetsPending.wait_for(std::chrono::seconds(0)) !=
        std::future_status::ready) {
        return false;
    }
    try {
        state.assets = state.assetsPending.get();
        // Every core but one prepares archetypes; this thread uploads.
        const unsigned cores =
            std::max(2u, std::thread::hardware_concurrency());
        state.pool = std::make_unique<Gta5LoadPool>(state.assets,
                                                    state.resources, cores - 1);
    } catch (const std::exception& ex) {
        // get() has consumed the future, so this is reported once and the
        // package falls back to tile files.
        if (logger) {
            logger->Warn(std::string("gta5.assets.index: failed: ") +
                         ex.what());
        }
        return false;
    }
    if (logger) {
        const Gta5AssetIndex& a = *state.assets;
        std::size_t hi = 0;  // resolved to a full-size +hi.ytd copy
        for (const auto& [hash, file] : a.textures) {
            hi += a.files[file].find("+hi.ytd") != std::string::npos;
        }
        logger->Info("gta5.assets.index: " + std::to_string(a.files.size()) +
                     " files, " + std::to_string(a.ymapCount) +
                     " ymaps over " + std::to_string(a.ymapsByTile.size()) +
                     " tiles, " + std::to_string(a.drawables.size()) +
                     " drawables, " + std::to_string(a.textures.size()) +
                     " textures (" + std::to_string(hi) + " +hi), in " +
                     std::to_string(a.buildSeconds) +
                     " s; " + std::to_string(state.pool->Threads()) +
                     " load threads");
    }
    return true;
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"

#include "services/interfaces/workflow/gta5/gta5_asset_scan.hpp"

#include <algorithm>
#include <future>
#include <thread>

namespace sdl3cpp::services::impl {

void ScanGta5AssetFiles(Gta5AssetIndex& index,
                        const std::vector<std::uint32_t>& ids,
                        const Gta5WorldConfig& world) {
    // Opening 30,000 files is disk- and inflate-bound, and each file is
    // independent, so every core takes an interleaved share.
    const std::size_t workers =
        std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::future<Gta5ScanResult>> jobs;
    for (std::size_t w = 0; w < workers; ++w) {
        jobs.push_back(std::async(std::launch::async, ScanGta5AssetSlice,
                                  &index.files, &ids, w, workers, world));
    }
    // Merged in worker order, so which duplicate wins is repeatable.
    for (auto& job : jobs) {
        const Gta5ScanResult r = job.get();
        for (const auto& d : r.drawables) index.drawables.emplace(d);
        for (const auto& t : r.textures) index.textures.emplace(t);
        for (const auto& [tile, file] : r.ymapTiles) {
            index.ymapsByTile[tile].push_back(file);
        }
        index.ymapCount += r.ymaps;
    }
}

}  // namespace sdl3cpp::services::impl

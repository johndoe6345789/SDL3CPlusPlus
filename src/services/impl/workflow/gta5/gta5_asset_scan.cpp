#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"

#include "services/interfaces/workflow/gta5/gta5_asset_scan.hpp"

#include <algorithm>
#include <future>
#include <string>
#include <thread>
#include <unordered_map>

namespace sdl3cpp::services::impl {
namespace {

bool IsHiTxd(const std::string& path) {
    return path.find("+hi.ytd") != std::string::npos;
}

}  // namespace

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
    std::unordered_map<std::uint32_t, std::uint32_t> sizes;
    // Merged in worker order, so which duplicate wins is repeatable.
    for (auto& job : jobs) {
        const Gta5ScanResult r = job.get();
        for (const auto& d : r.drawables) index.drawables.emplace(d);
        for (std::size_t i = 0; i < r.textures.size(); ++i) {
            // The largest copy of a name wins, a +hi.ytd on a tie: a LOD
            // dictionary's shrunk copy smeared whatever it landed on.
            const auto [hash, file] = r.textures[i];
            const std::uint32_t pixels = r.texturePixels[i];
            const auto [at, fresh] = index.textures.emplace(hash, file);
            std::uint32_t& best = sizes[hash];
            if (fresh || pixels > best ||
                (pixels == best && IsHiTxd(index.files[file]) &&
                 !IsHiTxd(index.files[at->second]))) {
                at->second = file;
                best = pixels;
            }
        }
        for (const auto& [tile, file] : r.ymapTiles) {
            index.ymapsByTile[tile].push_back(file);
        }
        index.ymapCount += r.ymaps;
    }
}

}  // namespace sdl3cpp::services::impl

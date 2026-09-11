#include "services/interfaces/workflow/gta5/gta5_asset_index.hpp"

#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <filesystem>

namespace sdl3cpp::services::impl {
namespace {

std::string Lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });
    return text;
}

/// Grass, LOD-light and occlusion ymaps place nothing drawable; grass
/// alone is 315 files of instance data.
bool SkipYmap(const std::string& path) {
    return path.find("_grass_") != std::string::npos ||
           path.find("lodlights") != std::string::npos ||
           path.find("_occl") != std::string::npos;
}

}  // namespace

Gta5AssetIndex BuildGta5AssetIndex(const std::string& root,
                                   const Gta5WorldConfig& world) {
    namespace fs = std::filesystem;
    const auto start = std::chrono::steady_clock::now();
    Gta5AssetIndex index;
    std::vector<std::uint32_t> opened;
    std::error_code ec;
    fs::recursive_directory_iterator it(
        root, fs::directory_options::skip_permission_denied, ec);
    for (; !ec && it != fs::recursive_directory_iterator(); it.increment(ec)) {
        if (!it->is_regular_file(ec)) continue;
        const std::string ext = Lower(it->path().extension().string());
        const bool named = ext == ".ydr" || ext == ".yft";
        const bool open = ext == ".ydd" || ext == ".ytd" ||
                          (ext == ".ymap" &&
                           !SkipYmap(Lower(it->path().string())));
        if (!named && !open) continue;
        const auto id = static_cast<std::uint32_t>(index.files.size());
        index.files.push_back(it->path().string());
        if (open) {
            opened.push_back(id);
            continue;
        }
        const std::string stem = Lower(it->path().stem().string());
        index.drawables.emplace(Gta5Hash(stem), id);
        index.names.emplace(Gta5Hash(stem), stem);
    }
    ScanGta5AssetFiles(index, opened, world);
    index.buildSeconds = std::chrono::duration<double>(
                             std::chrono::steady_clock::now() - start)
                             .count();
    return index;
}

std::string Gta5ArchetypeName(const Gta5AssetIndex& index,
                              std::uint32_t hash) {
    const auto found = index.names.find(hash);
    if (found != index.names.end()) return found->second;
    char text[16];
    std::snprintf(text, sizeof(text), "hash_%08X", hash);
    return text;
}

}  // namespace sdl3cpp::services::impl

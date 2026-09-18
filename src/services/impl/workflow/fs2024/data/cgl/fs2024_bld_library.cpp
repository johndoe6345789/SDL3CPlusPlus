#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_bld_library.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"

#include <filesystem>
#include <utility>

namespace sdl3cpp::fs2024 {
namespace {

constexpr int kBaseKeyDigits = 6;  ///< folder (3) + file name (3)
constexpr int kBldVersion = 6;

/// The tile's key within its container: the quad digits below the six
/// its file name already spells out, read as a base-4 number.
std::uint32_t KeyBelowContainer(const std::string& quadKey) {
    std::uint32_t key = 0;
    for (std::size_t i = kBaseKeyDigits; i < quadKey.size(); ++i) {
        key = key * 4 + static_cast<std::uint32_t>(quadKey[i] - '0');
    }
    return key;
}

}  // namespace

BldLibrary::BldLibrary(std::string cglRoot) : cglRoot_(std::move(cglRoot)) {}
BldLibrary::~BldLibrary() = default;

std::shared_ptr<const CglContainer> BldLibrary::Container(
    const std::string& baseKey, const std::string& kind) {
    const std::string path = cglRoot_ + "/CGL/" + baseKey.substr(0, 3) + "/" +
                            kind + baseKey.substr(3, 3) + ".cgl";
    return containers_.Get(kind + baseKey, [&] {
        return std::filesystem::exists(path)
                   ? std::make_shared<const CglContainer>(
                         ReadCglContainer(path))
                   : nullptr;
    });
}

std::vector<BldTile> BldLibrary::ReadTile(const QuadTile& tile) {
    const std::string quadKey = QuadKeyOf(tile);
    if (quadKey.size() <= kBaseKeyDigits) return {};
    const std::string baseKey = quadKey.substr(0, kBaseKeyDigits);

    std::vector<BldTile> decoded;
    for (const char* kind : {"bldo", "bldn"}) {
        const auto container = Container(baseKey, kind);
        if (!container) continue;
        const CglTileEntry* entry =
            FindCglTile(*container, KeyBelowContainer(quadKey));
        if (!entry) continue;
        decoded.push_back(
            DecodeBldTile(ReadCglTile(*container, *entry), kBldVersion));
    }
    return decoded;
}

}  // namespace sdl3cpp::fs2024

#include "services/interfaces/workflow/fs2024/data/vec/fs2024_vec_library.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"

#include <filesystem>
#include <utility>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::size_t kBaseKeyDigits = 6;  ///< folder (3) + file name (3)

/// A pyramid tile's key: the pair count of quad digits below the file's
/// own six, times 0x2000, in the top half; the digits as a base-4
/// number in the bottom.
std::uint32_t PyramidKey(const std::string& quadKey) {
    std::uint32_t index = 0;
    for (std::size_t i = kBaseKeyDigits; i < quadKey.size(); ++i) {
        index = index * 4 + static_cast<std::uint32_t>(quadKey[i] - '0');
    }
    const auto pairs =
        static_cast<std::uint32_t>((quadKey.size() - kBaseKeyDigits) / 2);
    return (pairs * 0x2000u) << 16 | index;
}

}  // namespace

VecLibrary::VecLibrary(std::string cglRoot) : cglRoot_(std::move(cglRoot)) {}

std::shared_ptr<const CglContainer> VecLibrary::Container(
    const std::string& baseKey) {
    const std::string path = cglRoot_ + "/CGL/" + baseKey.substr(0, 3) +
                             "/vec" + baseKey.substr(3, 3) + ".cgl";
    return containers_.Get(baseKey, [&] {
        return std::filesystem::exists(path)
                   ? std::make_shared<const CglContainer>(
                         ReadCglContainer(path))
                   : nullptr;
    });
}

VecTile VecLibrary::ReadTile(const QuadTile& tile) {
    const std::string quadKey = QuadKeyOf(tile);
    if (quadKey.size() <= kBaseKeyDigits) return {};
    const auto container = Container(quadKey.substr(0, kBaseKeyDigits));
    if (!container) return {};
    const CglTileEntry* entry = FindCglTile(*container, PyramidKey(quadKey));
    if (!entry) return {};
    return DecodeVecTile(ReadCglTile(*container, *entry));
}

}  // namespace sdl3cpp::fs2024

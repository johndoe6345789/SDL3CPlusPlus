#include "services/interfaces/workflow/fs2024/assemble/fs2024_cgl_pyramid.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_quadkey.hpp"

#include <filesystem>
#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kFileLevel = 6;

}  // namespace

Fs2024CglPyramid::Fs2024CglPyramid(std::string cglRoot, std::string kind)
    : cglRoot_(std::move(cglRoot)), kind_(std::move(kind)) {}

std::shared_ptr<const sdl3cpp::fs2024::CglContainer>
Fs2024CglPyramid::Container(const std::string& base) {
    const std::string path = cglRoot_ + "/CGL/" + base.substr(0, 3) + "/" +
                             kind_ + base.substr(3, 3) + ".cgl";
    return containers_.Get(base, [&] {
        return std::filesystem::exists(path)
                   ? std::make_shared<const sdl3cpp::fs2024::CglContainer>(
                         sdl3cpp::fs2024::ReadCglContainer(path))
                   : nullptr;
    });
}

std::vector<std::uint8_t> Fs2024CglPyramid::Read(int level, int x, int y) {
    if (level < kFileLevel) return {};
    const std::string key =
        sdl3cpp::fs2024::QuadKeyOf(sdl3cpp::fs2024::QuadTile{x, y, level});
    const auto container = Container(key.substr(0, kFileLevel));
    if (!container) return {};
    std::uint32_t index = 0;
    for (std::size_t i = kFileLevel; i < key.size(); ++i) {
        index = index * 4 + static_cast<std::uint32_t>(key[i] - '0');
    }
    const auto* tile = sdl3cpp::fs2024::FindCglTile(
        *container,
        (static_cast<std::uint32_t>(level - kFileLevel) << 12) | index);
    return tile ? sdl3cpp::fs2024::ReadCglTile(*container, *tile)
                : std::vector<std::uint8_t>{};
}

}  // namespace sdl3cpp::services::impl

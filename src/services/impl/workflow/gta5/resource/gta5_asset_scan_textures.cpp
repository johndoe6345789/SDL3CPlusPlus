#include "services/interfaces/workflow/gta5/resource/gta5_asset_scan.hpp"

namespace sdl3cpp::services::impl {

void ScanGta5Textures(const Gta5Resource& res, std::uint32_t id,
                      Gta5ScanResult& out) {
    // Name hashes at +0x20, count at +0x28, the textures themselves in the
    // same order at +0x30; each keeps width and height (u16) at +0x18. All
    // of it is in the system pages, so nothing is inflated to read sizes.
    const std::int64_t hashes = res.Follow(0x20);
    const std::int64_t textures = res.Follow(0x30);
    for (std::uint16_t i = 0; hashes >= 0 && i < res.U16(0x28); ++i) {
        const std::int64_t at = 8 * std::int64_t{i};
        const std::int64_t texture =
            textures < 0 ? -1 : res.Follow(textures + at);
        const std::uint32_t pixels =
            texture < 0 ? 0u
                        : std::uint32_t{res.U16(texture + 0x18)} *
                              res.U16(texture + 0x1A);
        out.textures.emplace_back(res.U32(hashes + 4 * std::int64_t{i}), id);
        out.texturePixels.push_back(pixels);
    }
}

}  // namespace sdl3cpp::services::impl

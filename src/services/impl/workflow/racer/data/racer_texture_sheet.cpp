#include "services/interfaces/workflow/racer/data/racer_asset_export.hpp"

#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"

#include <cstddef>

namespace sdl3cpp::services::impl {

bool WriteRacerTextureSheet(const std::vector<RacerTexture>& textures,
                            const std::filesystem::path& path) {
    if (textures.empty()) return false;
    // Every texture is resampled (nearest) into a 64x64 cell.
    constexpr int kColumns = 32;
    constexpr int kCell = 64;
    const int rows =
        static_cast<int>((textures.size() + kColumns - 1) / kColumns);
    const int width = kColumns * kCell;
    const int height = rows * kCell;
    std::vector<std::uint8_t> sheet(
        4 * static_cast<std::size_t>(width) * height, 0);
    for (std::size_t i = 0; i < textures.size(); ++i) {
        const RacerTexture& t = textures[i];
        const int ox = static_cast<int>(i % kColumns) * kCell;
        const int oy = static_cast<int>(i / kColumns) * kCell;
        for (int y = 0; y < kCell; ++y) {
            for (int x = 0; x < kCell; ++x) {
                const int sx = x * t.width / kCell;
                const int sy = y * t.height / kCell;
                const std::size_t src =
                    4 * (static_cast<std::size_t>(sy) * t.width + sx);
                const std::size_t dst =
                    4 * (static_cast<std::size_t>(oy + y) * width + ox + x);
                for (int c = 0; c < 4; ++c) sheet[dst + c] = t.rgba[src + c];
                sheet[dst + 3] = 255;
            }
        }
    }
    return WriteRacerPng(path, width, height, sheet);
}

}  // namespace sdl3cpp::services::impl

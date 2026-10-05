#include "services/interfaces/workflow/racer/data/racer_asset_io.hpp"
#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

#include <algorithm>
#include <cstddef>

namespace sdl3cpp::services::impl {

bool WriteRacerTextureSheet(const std::vector<RacerTexture>& textures,
                            const std::filesystem::path& path) {
    if (textures.empty()) return false;
    constexpr int kColumns = 25;
    const int cell = 64;
    const int rows = static_cast<int>(
        (textures.size() + kColumns - 1) / kColumns);
    const int width = kColumns * cell;
    const int height = rows * cell;
    std::vector<std::uint8_t> sheet(
        4 * static_cast<std::size_t>(width) * height, 0);
    for (std::size_t i = 0; i < textures.size(); ++i) {
        const int originX = static_cast<int>(i % kColumns) * cell;
        const int originY = static_cast<int>(i / kColumns) * cell;
        const auto& rgba = textures[i].rgba;
        for (int y = 0; y < cell; ++y) {
            const std::size_t src = 4 * static_cast<std::size_t>(y) * cell;
            const std::size_t dst =
                4 * (static_cast<std::size_t>(originY + y) * width + originX);
            std::copy_n(&rgba[src], 4 * cell, &sheet[dst]);
        }
    }
    return WriteRacerPng(path, width, height, sheet);
}

}  // namespace sdl3cpp::services::impl

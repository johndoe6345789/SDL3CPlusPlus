#include "services/interfaces/workflow/racer/data/racer_texture.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::uint8_t Expand5(std::uint32_t v) {
    return static_cast<std::uint8_t>((v * 255) / 31);
}

}  // namespace

std::vector<std::array<std::uint8_t, 4>> DecodeRacerPalette(
    const std::vector<std::uint8_t>& raw, std::size_t count) {
    std::vector<std::array<std::uint8_t, 4>> out;
    for (std::size_t i = 0; i < count && 2 * i + 1 < raw.size(); ++i) {
        const std::uint32_t v = (raw[2 * i] << 8) | raw[2 * i + 1];
        out.push_back({Expand5((v >> 11) & 31), Expand5((v >> 6) & 31),
                       Expand5((v >> 1) & 31),
                       static_cast<std::uint8_t>((v & 1) ? 255 : 0)});
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

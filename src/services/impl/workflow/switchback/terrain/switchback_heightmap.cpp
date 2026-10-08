#include "services/interfaces/workflow/switchback/terrain/switchback_heightmap.hpp"

#include <cmath>
#include <cstddef>
#include <fstream>

namespace sdl3cpp::services::impl {

bool ReadSwitchbackHeightmap(const std::string& path, float heightMaxM,
                             SwitchbackHeightmap& out) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) return false;
    const std::streamsize bytes = file.tellg();
    if (bytes <= 0 || bytes % 2 != 0) return false;
    std::vector<unsigned char> raw(static_cast<std::size_t>(bytes));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(raw.data()), bytes);
    if (!file) return false;

    const std::size_t samples = raw.size() / 2;
    const auto side = static_cast<std::size_t>(
        std::sqrt(static_cast<double>(samples)) + 0.5);
    if (side < 2 || side * side != samples) return false;

    out.size = static_cast<int>(side);
    out.metres.resize(samples);
    for (std::size_t i = 0; i < samples; ++i) {
        const unsigned value =
            raw[2 * i] | (static_cast<unsigned>(raw[2 * i + 1]) << 8);
        out.metres[i] = static_cast<float>(value) / 65535.f * heightMaxM;
    }
    return true;
}

}  // namespace sdl3cpp::services::impl

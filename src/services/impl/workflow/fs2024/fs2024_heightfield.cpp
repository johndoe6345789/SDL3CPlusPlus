#include "services/interfaces/workflow/fs2024/fs2024_heightfield.hpp"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::services::impl {
namespace {

struct FileHeader {
    char magic[4];
    std::uint32_t columns;
    std::uint32_t rows;
    float spacing;
    float originX;
    float originZ;
};

[[noreturn]] void Fail(const std::string& path, const std::string& why) {
    throw std::runtime_error("fs2024 heightfield '" + path + "': " + why);
}

}  // namespace

Fs2024Heightfield ReadFs2024Heightfield(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) Fail(path, "cannot open");

    FileHeader header{};
    in.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (!in || std::memcmp(header.magic, "FST1", 4) != 0) {
        Fail(path, "not an FST1 file");
    }
    if (header.columns < 2 || header.rows < 2 || header.spacing <= 0.f) {
        Fail(path, "degenerate grid");
    }

    Fs2024Heightfield field;
    field.columns = static_cast<int>(header.columns);
    field.rows = static_cast<int>(header.rows);
    field.spacing = header.spacing;
    field.origin = {header.originX, header.originZ};
    field.heights.resize(static_cast<std::size_t>(header.columns) *
                         header.rows);
    const auto bytes = static_cast<std::streamsize>(
        field.heights.size() * sizeof(float));
    in.read(reinterpret_cast<char*>(field.heights.data()), bytes);
    if (in.gcount() != bytes) Fail(path, "truncated");

    const auto [low, high] =
        std::minmax_element(field.heights.begin(), field.heights.end());
    field.minHeight = *low;
    field.maxHeight = *high;
    return field;
}

float Fs2024HeightAt(const Fs2024Heightfield& field, float x, float z) {
    const float gx = std::clamp((x - field.origin.x) / field.spacing, 0.f,
                                static_cast<float>(field.columns - 1));
    const float gz = std::clamp((z - field.origin.y) / field.spacing, 0.f,
                                static_cast<float>(field.rows - 1));
    const int c = std::min(static_cast<int>(gx), field.columns - 2);
    const int r = std::min(static_cast<int>(gz), field.rows - 2);
    const float fx = gx - static_cast<float>(c);
    const float fz = gz - static_cast<float>(r);
    // Each cell is split along (c+1, r)-(c, r+1), as Bullet's
    // btHeightfieldTerrainShape splits it by default.
    const float h10 = field.At(c + 1, r);
    const float h01 = field.At(c, r + 1);
    if (fx + fz <= 1.f) {
        const float h00 = field.At(c, r);
        return h00 + (h10 - h00) * fx + (h01 - h00) * fz;
    }
    const float h11 = field.At(c + 1, r + 1);
    return h11 + (h01 - h11) * (1.f - fx) + (h10 - h11) * (1.f - fz);
}

}  // namespace sdl3cpp::services::impl

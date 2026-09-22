#include "services/interfaces/workflow/stunts/data/stunts_shape_archive.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t ReadU32(const std::vector<std::uint8_t>& d, std::size_t at) {
    return static_cast<std::uint32_t>(d[at]) |
           (static_cast<std::uint32_t>(d[at + 1]) << 8) |
           (static_cast<std::uint32_t>(d[at + 2]) << 16) |
           (static_cast<std::uint32_t>(d[at + 3]) << 24);
}

std::uint16_t ReadU16(const std::vector<std::uint8_t>& d, std::size_t at) {
    return static_cast<std::uint16_t>(d[at] | (d[at + 1] << 8));
}

}  // namespace

StuntsShapeArchive ParseStuntsShapeArchive(
    const std::vector<std::uint8_t>& decoded) {
    StuntsShapeArchive out;
    if (decoded.size() < 6) return out;
    const std::uint32_t size = ReadU32(decoded, 0);
    const std::uint16_t count = ReadU16(decoded, 4);
    if (count == 0 || size > decoded.size()) return out;

    const std::size_t tagsAt = 6;
    const std::size_t offsetsAt = tagsAt + 4u * count;
    const std::size_t base = offsetsAt + 4u * count;
    if (base > decoded.size()) return out;

    for (std::uint16_t i = 0; i < count; ++i) {
        const auto* tagBytes =
            reinterpret_cast<const char*>(&decoded[tagsAt + 4u * i]);
        std::string tag(tagBytes, 4);
        const std::uint32_t offset = ReadU32(decoded, offsetsAt + 4u * i);
        const std::uint32_t nextOffset =
            (i + 1 < count) ? ReadU32(decoded, offsetsAt + 4u * (i + 1))
                            : static_cast<std::uint32_t>(size - base);
        const std::size_t start = base + offset;
        const std::size_t end = base + nextOffset;
        if (start > end || end > decoded.size()) continue;
        out[tag].assign(decoded.begin() + static_cast<long>(start),
                        decoded.begin() + static_cast<long>(end));
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

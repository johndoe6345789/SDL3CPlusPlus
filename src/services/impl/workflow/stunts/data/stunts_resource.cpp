#include "services/interfaces/workflow/stunts/data/stunts_resource.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t ReadU32(const std::vector<std::uint8_t>& raw, std::size_t at) {
    return static_cast<std::uint32_t>(raw[at]) |
           (static_cast<std::uint32_t>(raw[at + 1]) << 8) |
           (static_cast<std::uint32_t>(raw[at + 2]) << 16) |
           (static_cast<std::uint32_t>(raw[at + 3]) << 24);
}

std::uint16_t ReadU16(const std::vector<std::uint8_t>& raw, std::size_t at) {
    return static_cast<std::uint16_t>(raw[at] | (raw[at + 1] << 8));
}

}  // namespace

StuntsResources ParseStuntsResources(const std::vector<std::uint8_t>& raw) {
    StuntsResources out;
    if (raw.size() < 6) return out;
    const std::uint32_t size = ReadU32(raw, 0);
    const std::uint16_t count = ReadU16(raw, 4);
    if (size > raw.size() || count == 0) return out;

    const std::size_t tagsAt = 6;
    const std::size_t offsetsAt = tagsAt + 4u * count;
    const std::size_t base = offsetsAt + 4u * count;
    if (base > raw.size()) return out;

    std::vector<std::string> tags(count);
    std::vector<std::uint32_t> offsets(count);
    for (std::uint16_t i = 0; i < count; ++i) {
        tags[i].assign(reinterpret_cast<const char*>(&raw[tagsAt + 4u * i]), 4);
        offsets[i] = ReadU32(raw, offsetsAt + 4u * i);
    }

    std::vector<std::uint16_t> order(count);
    for (std::uint16_t i = 0; i < count; ++i) order[i] = i;
    std::sort(order.begin(), order.end(),
              [&](std::uint16_t a, std::uint16_t b) {
                  return offsets[a] < offsets[b];
              });

    for (std::size_t rank = 0; rank < order.size(); ++rank) {
        const std::size_t start = base + offsets[order[rank]];
        const std::size_t end = rank + 1 < order.size()
                                    ? base + offsets[order[rank + 1]]
                                    : size;
        if (start > end || end > raw.size()) continue;
        out[tags[order[rank]]].assign(raw.begin() + static_cast<long>(start),
                                      raw.begin() + static_cast<long>(end));
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

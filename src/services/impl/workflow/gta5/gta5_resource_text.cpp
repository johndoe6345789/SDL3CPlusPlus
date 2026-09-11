#include "services/interfaces/workflow/gta5/gta5_resource.hpp"

#include <cctype>

namespace sdl3cpp::services::impl {

std::string Gta5Resource::String(std::int64_t at) const {
    std::string out;
    // Names are short; the cap stops a bad pointer reading the whole file.
    for (std::int64_t i = at; at >= 0 && i < at + 256; ++i) {
        const std::uint8_t c = U8(i);
        if (c == 0) break;
        out.push_back(static_cast<char>(c));
    }
    return out;
}

std::uint32_t Gta5Hash(const std::string& text) {
    std::uint32_t h = 0;
    for (const unsigned char c : text) {
        h += static_cast<std::uint32_t>(std::tolower(c));
        h += h << 10;
        h ^= h >> 6;
    }
    h += h << 3;
    h ^= h >> 11;
    h += h << 15;
    return h;
}

}  // namespace sdl3cpp::services::impl

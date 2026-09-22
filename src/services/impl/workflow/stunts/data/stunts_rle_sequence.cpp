#include "services/impl/workflow/stunts/data/stunts_rle_sequence.hpp"

namespace sdl3cpp::services::impl {

std::vector<std::uint8_t> ExpandStuntsSequences(
    const std::vector<std::uint8_t>& body, std::uint8_t marker) {
    std::vector<std::uint8_t> out;
    out.reserve(body.size());
    std::size_t p = 0;
    const std::size_t n = body.size();
    while (p < n) {
        if (body[p] != marker) {
            out.push_back(body[p]);
            ++p;
            continue;
        }
        // Find the matching close: the next occurrence of `marker`.
        // A trailing, unclosed marker (or one with no count byte
        // after it) is not a sequence -- pass it through as data.
        std::size_t close = p + 1;
        while (close < n && body[close] != marker) ++close;
        if (close >= n || close + 1 >= n) {
            out.push_back(body[p]);
            ++p;
            continue;
        }
        const std::size_t contentStart = p + 1;
        const std::uint8_t count = body[close + 1];
        for (std::uint8_t copy = 0; copy < count; ++copy) {
            out.insert(out.end(),
                      body.begin() + static_cast<long>(contentStart),
                      body.begin() + static_cast<long>(close));
        }
        p = close + 2;
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

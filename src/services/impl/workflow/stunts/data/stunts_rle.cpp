#include "services/interfaces/workflow/stunts/data/stunts_rle.hpp"

#include "services/impl/workflow/stunts/data/stunts_rle_sequence.hpp"

#include <array>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint8_t kRlePassId = 1;
constexpr std::size_t kHeaderBytes = 9;  // id + size:24 + bodyLen:32
constexpr std::uint8_t kNoSequenceFlag = 0x80;
constexpr std::uint8_t kEscapeCountMask = 0x7f;

/// escapeIndex[byte] is that byte's escape position (0-9), or -1 if
/// the byte is not one of the ten escapes -- a plain literal.
std::array<int, 256> BuildEscapeIndex(const std::uint8_t* escapes,
                                      std::size_t count) {
    std::array<int, 256> index{};
    index.fill(-1);
    for (std::size_t i = 0; i < count; ++i) {
        index[escapes[i]] = static_cast<int>(i);
    }
    return index;
}

std::vector<std::uint8_t> ApplyEscapes(const std::vector<std::uint8_t>& body,
                                       const std::array<int, 256>& index,
                                       std::size_t outSize) {
    std::vector<std::uint8_t> out;
    out.reserve(outSize);
    std::size_t p = 0;
    const std::size_t n = body.size();
    while (out.size() < outSize && p < n) {
        const int i = index[body[p]];
        if (i < 0) {
            out.push_back(body[p]);
            ++p;
        } else if (i == 0) {
            if (p + 2 >= n) break;
            out.insert(out.end(), body[p + 1], body[p + 2]);
            p += 3;
        } else if (i == 2) {
            if (p + 3 >= n) break;
            const std::size_t count =
                static_cast<std::size_t>(body[p + 1]) |
                (static_cast<std::size_t>(body[p + 2]) << 8);
            out.insert(out.end(), count, body[p + 3]);
            p += 4;
        } else {
            if (p + 1 >= n) break;
            out.insert(out.end(), static_cast<std::size_t>(i), body[p + 1]);
            p += 2;
        }
    }
    return out;
}

}  // namespace

std::vector<std::uint8_t> DecodeStuntsRle(const std::uint8_t* data,
                                          std::size_t size) {
    if (size < kHeaderBytes || data[0] != kRlePassId) return {};
    const std::size_t outSize = static_cast<std::size_t>(data[1]) |
                                (static_cast<std::size_t>(data[2]) << 8) |
                                (static_cast<std::size_t>(data[3]) << 16);
    const std::size_t escapeCount = data[8] & kEscapeCountMask;
    const bool noSequences = (data[8] & kNoSequenceFlag) != 0;
    const std::size_t escapesAt = kHeaderBytes;
    const std::size_t bodyAt = escapesAt + escapeCount;
    if (bodyAt > size) return {};

    const auto index = BuildEscapeIndex(data + escapesAt, escapeCount);
    std::vector<std::uint8_t> body(data + bodyAt, data + size);
    if (!noSequences && escapeCount > 1) {
        body = ExpandStuntsSequences(body, data[escapesAt + 1]);
    }
    return ApplyEscapes(body, index, outSize);
}

}  // namespace sdl3cpp::services::impl

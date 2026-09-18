#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_table.hpp"

#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

class WordCursor {
public:
    explicit WordCursor(const std::vector<std::uint16_t>& words)
        : words_(words) {}
    std::int64_t Next() {
        if (at_ >= words_.size()) {
            throw std::runtime_error("CGL table: truncated");
        }
        return words_[at_++];
    }

private:
    const std::vector<std::uint16_t>& words_;
    std::size_t at_ = 0;
};

/// Compressed sizes: < 0x4000 adds, 0x4000..0x7fff subtracts
/// (0x8000 - v) -- 0x4000 itself is -16384, not +16384: read the other
/// way it sends every later size in a big file 32 KB too high --
/// 0x8000..0xfeff adds (v - 0x8000) * 65536 plus the next word, and
/// 0xff00.. subtracts whole 65536s the same way.
std::int64_t NextCompressed(WordCursor& cursor, std::int64_t last) {
    const std::int64_t v = cursor.Next();
    if (v >= 0xff00) {
        return last - 0x10000 * (0x10000 - v) + cursor.Next();
    }
    if (v >= 0x8000) return last + 0x10000 * (v - 0x8000) + cursor.Next();
    if (v >= 0x4000) return last - (0x8000 - v);
    return last + v;
}

/// Uncompressed sizes are the tile's compressed size plus a
/// non-negative extra, escaped the same way above 0x7fff.
std::int64_t NextUncompressed(WordCursor& cursor, std::int64_t base) {
    const std::int64_t v = cursor.Next();
    if (v >= 0x8000) return base + 0x10000 * (v - 0x8000) + cursor.Next();
    return base + v;
}

}  // namespace

std::vector<CglTileEntry> ParseCglTable(
    const std::vector<std::uint16_t>& words, std::size_t count,
    std::uint64_t dataStart) {
    WordCursor cursor(words);
    std::vector<CglTileEntry> tiles(count);
    std::uint32_t key = 0;
    for (CglTileEntry& tile : tiles) {
        key += static_cast<std::uint32_t>(cursor.Next());
        tile.key = key;
    }
    std::int64_t last = 0;
    for (CglTileEntry& tile : tiles) {
        last = NextCompressed(cursor, last);
        if (last < 0) throw std::runtime_error("CGL table: negative size");
        tile.compressedSize = static_cast<std::uint32_t>(last);
    }
    std::uint64_t offset = dataStart;
    for (CglTileEntry& tile : tiles) {
        tile.uncompressedSize = static_cast<std::uint32_t>(
            NextUncompressed(cursor, tile.compressedSize));
        tile.offset = offset;
        offset += tile.compressedSize;
    }
    return tiles;
}

}  // namespace sdl3cpp::fs2024

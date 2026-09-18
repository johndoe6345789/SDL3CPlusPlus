#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_lzma_raw.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint32_t kMagic = 0x41734246;  // "FBsA"
constexpr std::uint64_t kHeaderPeek = 0x34;

std::vector<std::uint8_t> ReadBytes(std::ifstream& in, std::uint64_t at,
                                    std::uint64_t size) {
    std::vector<std::uint8_t> bytes(size);
    in.seekg(static_cast<std::streamoff>(at));
    in.read(reinterpret_cast<char*>(bytes.data()),
           static_cast<std::streamsize>(size));
    if (!in) throw std::runtime_error("CGL: short read");
    return bytes;
}

}  // namespace

CglContainer ReadCglContainer(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("CGL '" + path + "': cannot open");
    const auto head = ReadBytes(in, 0, kHeaderPeek);
    std::uint32_t magic = 0, count = 0, tableAt = 0, tableSize = 0;
    std::memcpy(&magic, head.data(), 4);
    if (magic != kMagic) throw std::runtime_error("CGL: not FBsA");
    std::memcpy(&count, head.data() + 32, 4);
    std::memcpy(&tableAt, head.data() + 36, 4);
    std::memcpy(&tableSize, head.data() + 40, 4);
    tableSize &= 0xffffff;  // top byte holds flags

    CglContainer container;
    container.path = path;
    container.dataProps = head[48];
    const auto packed = ReadBytes(in, tableAt, tableSize);
    const auto table =
        DecodeLzmaRawAll(packed.data(), packed.size(), head[49]);
    std::vector<std::uint16_t> words(table.size() / 2);
    std::memcpy(words.data(), table.data(), words.size() * 2);
    container.tiles =
        ParseCglTable(words, count, std::uint64_t{tableAt} + tableSize);
    return container;
}

const CglTileEntry* FindCglTile(const CglContainer& container,
                                std::uint32_t key) {
    const auto it = std::lower_bound(
        container.tiles.begin(), container.tiles.end(), key,
        [](const CglTileEntry& tile, std::uint32_t k) { return tile.key < k; });
    return it != container.tiles.end() && it->key == key ? &*it : nullptr;
}

}  // namespace sdl3cpp::fs2024

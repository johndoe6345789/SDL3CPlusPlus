#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_cgl_container.hpp"

#include "services/interfaces/workflow/fs2024/data/cgl/fs2024_lzma_raw.hpp"

#include <fstream>
#include <stdexcept>
#include <string>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::uint8_t kSizelessProps = 0x5D;

}  // namespace

std::vector<std::uint8_t> ReadCglTile(const CglContainer& container,
                                      const CglTileEntry& tile) {
    std::ifstream in(container.path, std::ios::binary);
    in.seekg(0, std::ios::end);
    const auto fileSize = static_cast<std::uint64_t>(in.tellg());
    if (tile.offset + tile.compressedSize > fileSize) {
        throw std::runtime_error(
            "CGL '" + container.path + "': tile " + std::to_string(tile.key) +
            " runs past the file (" + std::to_string(tile.offset) + " + " +
            std::to_string(tile.compressedSize) + " > " +
            std::to_string(fileSize) + ")");
    }
    std::vector<std::uint8_t> packed(tile.compressedSize);
    in.seekg(static_cast<std::streamoff>(tile.offset));
    in.read(reinterpret_cast<char*>(packed.data()),
            static_cast<std::streamsize>(packed.size()));
    if (!in) throw std::runtime_error("CGL: short read");
    if (tile.compressedSize == tile.uncompressedSize) {
        // Equal sizes usually mean stored as-is -- but most of a dem
        // file's tiles record no size of their own and are packed all
        // the same, with LZMA's standard lc 3 / lp 0 / pb 2 rather than
        // the header's props. A range-coded stream always begins with a
        // zero byte, which a dem tile's own framing never does (vector
        // tiles, stored as-is, often do).
        if (!container.sizelessPacked || packed.empty() || packed[0] != 0) {
            return packed;
        }
        return DecodeLzmaRawAll(packed.data(), packed.size(),
                                kSizelessProps);
    }
    return DecodeLzmaRaw(packed.data(), packed.size(), container.dataProps,
                         tile.uncompressedSize);
}

}  // namespace sdl3cpp::fs2024

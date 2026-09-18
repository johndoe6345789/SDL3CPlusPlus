#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_texture.hpp"

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_bc_decode.hpp"

#include <cstring>
#include <fstream>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::size_t kHeaderSize = 128;  ///< "DDS " (4) + DDS_HEADER (124)

/// Reads and decodes every 4x4 block into an aligned RGBA8 buffer
/// (dimensions rounded up to a multiple of 4, as blocks require).
std::vector<std::uint8_t> DecodeBlocks(std::ifstream& in, bool isBc3,
                                      std::uint32_t blocksWide,
                                      std::uint32_t blocksHigh) {
    const std::size_t bytesPerBlock = isBc3 ? 16 : 8;
    std::vector<std::uint8_t> compressed(
        static_cast<std::size_t>(blocksWide) * blocksHigh * bytesPerBlock);
    in.read(reinterpret_cast<char*>(compressed.data()),
           static_cast<std::streamsize>(compressed.size()));
    if (!in) throw std::runtime_error("DDS: short block read");

    const int stride = static_cast<int>(blocksWide * 4 * 4);
    std::vector<std::uint8_t> aligned(
        static_cast<std::size_t>(stride) * blocksHigh * 4);
    for (std::uint32_t by = 0; by < blocksHigh; ++by) {
        for (std::uint32_t bx = 0; bx < blocksWide; ++bx) {
            const std::uint8_t* block =
                compressed.data() + (by * blocksWide + bx) * bytesPerBlock;
            std::uint8_t* dst = aligned.data() + by * 4 * stride + bx * 16;
            if (isBc3) DecodeBc3Block(block, dst, stride);
            else DecodeBc1Block(block, dst, stride);
        }
    }
    return aligned;
}

}  // namespace

DdsImage DecodeDds(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("DDS '" + path + "': cannot open");

    std::vector<std::uint8_t> header(kHeaderSize);
    in.read(reinterpret_cast<char*>(header.data()),
           static_cast<std::streamsize>(header.size()));
    if (!in || std::memcmp(header.data(), "DDS ", 4) != 0) {
        throw std::runtime_error("DDS '" + path + "': not a DDS file");
    }
    std::uint32_t height = 0, width = 0;
    std::memcpy(&height, header.data() + 12, 4);
    std::memcpy(&width, header.data() + 16, 4);
    char fourCc[5] = {};
    std::memcpy(fourCc, header.data() + 84, 4);

    const bool isBc3 = std::strcmp(fourCc, "DXT5") == 0;
    if (!isBc3 && std::strcmp(fourCc, "DXT1") != 0) {
        throw std::runtime_error("DDS '" + path + "': unsupported fourCC " +
                                 fourCc);
    }

    const std::uint32_t blocksWide = (width + 3) / 4;
    const std::uint32_t blocksHigh = (height + 3) / 4;
    const auto aligned = DecodeBlocks(in, isBc3, blocksWide, blocksHigh);
    const int alignedStride = static_cast<int>(blocksWide * 4 * 4);

    DdsImage image;
    image.width = static_cast<int>(width);
    image.height = static_cast<int>(height);
    image.rgba.resize(static_cast<std::size_t>(width) * height * 4);
    for (std::uint32_t y = 0; y < height; ++y) {
        std::memcpy(image.rgba.data() + y * width * 4,
                   aligned.data() + y * alignedStride, width * 4);
    }
    return image;
}

}  // namespace sdl3cpp::fs2024

#include "services/interfaces/workflow/fs2024/data/texture/fs2024_dds_blocks.hpp"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace sdl3cpp::fs2024 {
namespace {

constexpr std::size_t kHeaderBytes = 128;

std::uint32_t U32(const std::vector<std::uint8_t>& bytes, std::size_t at) {
    std::uint32_t value = 0;
    std::memcpy(&value, &bytes[at], 4);
    return value;
}

std::size_t MipBytes(const DdsBlocks& dds, std::uint32_t mip) {
    const std::uint32_t w = std::max(dds.width >> mip, 1u);
    const std::uint32_t h = std::max(dds.height >> mip, 1u);
    return std::size_t{(w + 3) / 4} * ((h + 3) / 4) * dds.blockBytes;
}

}  // namespace

DdsBlocks ReadDdsBlocks(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    const std::vector<std::uint8_t> file(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());
    if (file.size() < kHeaderBytes || std::memcmp(file.data(), "DDS ", 4)) {
        throw std::runtime_error("dds '" + path + "': not a DDS file");
    }
    DdsBlocks dds;
    dds.height = U32(file, 12);
    dds.width = U32(file, 16);
    const auto* fourCc = &file[84];
    if (std::memcmp(fourCc, "DXT5", 4) == 0) {
        dds.alpha = true;
        dds.blockBytes = 16;
    } else if (std::memcmp(fourCc, "DXT1", 4) != 0) {
        throw std::runtime_error("dds '" + path + "': not DXT1 or DXT5");
    }
    const std::uint32_t listed = std::max(U32(file, 28), 1u);
    std::size_t end = kHeaderBytes;
    dds.mips = 0;
    while (dds.mips < listed &&
           end + MipBytes(dds, dds.mips) <= file.size()) {
        end += MipBytes(dds, dds.mips++);
    }
    if (dds.mips == 0) throw std::runtime_error("dds '" + path + "': short");
    dds.data.assign(file.begin() + kHeaderBytes, file.begin() + end);
    return dds;
}

}  // namespace sdl3cpp::fs2024

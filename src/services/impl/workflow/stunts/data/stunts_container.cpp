#include "services/interfaces/workflow/stunts/data/stunts_container.hpp"

#include "services/interfaces/workflow/stunts/data/stunts_huffman.hpp"
#include "services/interfaces/workflow/stunts/data/stunts_rle.hpp"

#include <cstdio>

namespace sdl3cpp::services::impl {
namespace {

constexpr std::uint8_t kMorePasses = 0x80;
constexpr std::uint8_t kHuffman = 2;
constexpr std::uint8_t kRlePassId = 1;
constexpr std::size_t kHeaderBytes = 4;

struct Pass {
    std::uint8_t type = 0;
    std::size_t size = 0;
};

/// True when `raw` opens with a pass this build can decode. Files with
/// no container (.TRK, .RES) fail this and are passed through as-is.
bool LooksCompressed(const std::vector<std::uint8_t>& raw) {
    return raw.size() > kHeaderBytes &&
           (raw[0] & ~kMorePasses) == kHuffman;
}

std::vector<Pass> ReadPasses(const std::vector<std::uint8_t>& raw,
                             std::size_t& payloadAt) {
    std::vector<Pass> passes;
    std::size_t at = 0;
    while (at + kHeaderBytes <= raw.size()) {
        const std::uint8_t flags = raw[at];
        Pass pass;
        pass.type = static_cast<std::uint8_t>(flags & ~kMorePasses);
        pass.size = static_cast<std::size_t>(raw[at + 1]) |
                    (static_cast<std::size_t>(raw[at + 2]) << 8) |
                    (static_cast<std::size_t>(raw[at + 3]) << 16);
        passes.push_back(pass);
        at += kHeaderBytes;
        if (!(flags & kMorePasses)) break;
    }
    payloadAt = at;
    return passes;
}

}  // namespace

std::vector<std::uint8_t> DecodeStuntsContainer(
    const std::vector<std::uint8_t>& raw) {
    if (!LooksCompressed(raw)) return raw;

    std::size_t payloadAt = 0;
    const std::vector<Pass> passes = ReadPasses(raw, payloadAt);
    if (passes.empty() || payloadAt >= raw.size()) return {};

    // The innermost pass is the one the payload was written with; the
    // headers list the passes outermost first.
    const Pass& inner = passes.back();
    if (inner.type != kHuffman) return {};
    const StuntsHuffmanTable table =
        ReadStuntsHuffmanTable(raw.data() + payloadAt, raw.size() - payloadAt);
    if (!table.valid) return {};
    const std::vector<std::uint8_t> huffmanOut = DecodeStuntsHuffman(
        table, raw.data() + payloadAt, raw.size() - payloadAt, inner.size);

    // The larger files nest a run-length pass inside the Huffman
    // output rather than stacking its header before it. A leading
    // pass-1 id there is that nested header, not game data.
    if (!huffmanOut.empty() && huffmanOut[0] == kRlePassId) {
        std::vector<std::uint8_t> rleOut =
            DecodeStuntsRle(huffmanOut.data(), huffmanOut.size());
        if (!rleOut.empty()) return rleOut;
    }
    return huffmanOut;
}

std::vector<std::uint8_t> LoadStuntsFile(const std::string& path) {
    std::vector<std::uint8_t> raw;
    std::FILE* file = std::fopen(path.c_str(), "rb");
    if (!file) return raw;
    std::fseek(file, 0, SEEK_END);
    const long size = std::ftell(file);
    std::fseek(file, 0, SEEK_SET);
    if (size > 0) {
        raw.resize(static_cast<std::size_t>(size));
        if (std::fread(raw.data(), 1, raw.size(), file) != raw.size()) {
            raw.clear();
        }
    }
    std::fclose(file);
    return DecodeStuntsContainer(raw);
}

}  // namespace sdl3cpp::services::impl

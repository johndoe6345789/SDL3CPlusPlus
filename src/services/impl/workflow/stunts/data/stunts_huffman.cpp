#include "services/interfaces/workflow/stunts/data/stunts_huffman.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// A complete prefix code spends its whole space: sum(2^-length) == 1.
bool IsComplete(const std::vector<std::uint32_t>& count) {
    double space = 0.0;
    for (std::size_t length = 1; length < count.size(); ++length) {
        space += static_cast<double>(count[length]) /
                 static_cast<double>(1u << length);
    }
    return space > 0.999 && space < 1.001;
}

}  // namespace

StuntsHuffmanTable ReadStuntsHuffmanTable(const std::uint8_t* data,
                                          std::size_t size) {
    StuntsHuffmanTable table;
    if (size < 2) return table;
    // One count byte per code length, lengths 1..widths. Reading one
    // byte too many here swallows the first symbol and leaves the code
    // over-subscribed, which IsComplete() then rejects.
    const std::size_t widths = data[0];
    if (size < widths + 1) return table;

    table.count.assign(widths + 1, 0);
    std::size_t total = 0;
    for (std::size_t i = 0; i < widths; ++i) {
        table.count[i + 1] = data[1 + i];
        total += data[1 + i];
    }
    const std::size_t symbolsAt = widths + 1;
    if (size < symbolsAt + total || total == 0) return table;
    table.symbols.assign(data + symbolsAt, data + symbolsAt + total);
    table.headerBytes = symbolsAt + total;

    table.firstCode.assign(widths + 1, 0);
    table.firstIndex.assign(widths + 1, 0);
    std::uint32_t code = 0;
    std::uint32_t index = 0;
    for (std::size_t length = 1; length <= widths; ++length) {
        table.firstCode[length] = code;
        table.firstIndex[length] = index;
        code += table.count[length];
        index += table.count[length];
        code <<= 1;
    }
    table.valid = IsComplete(table.count);
    return table;
}

std::vector<std::uint8_t> DecodeStuntsHuffman(const StuntsHuffmanTable& table,
                                              const std::uint8_t* data,
                                              std::size_t size,
                                              std::size_t outSize) {
    std::vector<std::uint8_t> out;
    if (!table.valid || size <= table.headerBytes) return out;
    out.reserve(outSize);

    const std::uint8_t* bits = data + table.headerBytes;
    const std::size_t bitCount = (size - table.headerBytes) * 8;
    std::uint32_t code = 0;
    std::size_t length = 0;
    for (std::size_t at = 0; at < bitCount && out.size() < outSize; ++at) {
        code = (code << 1) | ((bits[at >> 3] >> (7 - (at & 7))) & 1u);
        ++length;
        if (length >= table.count.size()) break;
        const std::uint32_t first = table.firstCode[length];
        if (table.count[length] == 0 || code < first ||
            code - first >= table.count[length]) {
            continue;
        }
        out.push_back(table.symbols[table.firstIndex[length] + code - first]);
        code = 0;
        length = 0;
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

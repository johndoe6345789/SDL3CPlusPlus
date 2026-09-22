#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// One decoded canonical Huffman table plus the byte it ends on.
struct StuntsHuffmanTable {
    /// Symbol for each (code length, code) pair, flattened by length:
    /// codes[length] holds the first code of that length and
    /// symbols[length] the index of its first symbol.
    std::vector<std::uint32_t> firstCode;
    std::vector<std::uint32_t> firstIndex;
    std::vector<std::uint32_t> count;
    std::vector<std::uint8_t> symbols;
    std::size_t headerBytes = 0;
    bool valid = false;
};

/// Reads the table at the head of `data`; `valid` is false when the
/// counts run past the buffer or do not form a complete code.
StuntsHuffmanTable ReadStuntsHuffmanTable(const std::uint8_t* data,
                                          std::size_t size);

/// Decodes exactly `outSize` bytes from the bit stream that follows
/// `table` in `data`, MSB first.
std::vector<std::uint8_t> DecodeStuntsHuffman(const StuntsHuffmanTable& table,
                                              const std::uint8_t* data,
                                              std::size_t size,
                                              std::size_t outSize);

}  // namespace sdl3cpp::services::impl

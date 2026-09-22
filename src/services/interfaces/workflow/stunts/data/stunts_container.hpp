#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Decodes a Stunts container file into its stored bytes.
 *
 * A compressed Stunts file opens with a four-byte header: a flags
 * byte whose low seven bits name the pass and whose top bit says
 * another header follows, then that pass's output size as a
 * little-endian 24-bit count. Pass 2 is a canonical Huffman code --
 * one count byte per code length starting at length one, the symbols
 * in code order, then an MSB-first bit stream.
 *
 * The larger .P3S/.PVS files nest a second pass (type 1, run-length)
 * inside the Huffman output rather than stacking a second header
 * before it: the Huffman pass's own decoded bytes open with their own
 * small header, in the same [id][size] shape, and DecodeStuntsRle
 * unpacks that. This function detects and chains that automatically.
 *
 * Files with no header (.TRK tracks, .RES archives) are returned
 * unchanged, so callers may hand any game file to this.
 *
 * @return The decoded bytes, or an empty vector if `raw` is truncated
 *         or names a pass this does not implement.
 */
std::vector<std::uint8_t> DecodeStuntsContainer(
    const std::vector<std::uint8_t>& raw);

/// Reads `path` and runs it through DecodeStuntsContainer().
std::vector<std::uint8_t> LoadStuntsFile(const std::string& path);

}  // namespace sdl3cpp::services::impl

#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/**
 * @brief Decodes the run-length pass that follows Huffman in a Stunts
 *        container.
 *
 * The Huffman output is itself a small container: a one-byte pass id
 * (1, for this pass), a 24-bit final output size, a 32-bit length of
 * what follows, one byte holding the escape count in its low 7 bits
 * and a "no sequences" flag in its top bit, then that many escape
 * byte values, then the run-length body.
 *
 * Ten escape codes are used throughout the game's files. Escape 0 is
 * `[count][value]`, escape 2 is `[count_lo][count_hi][value]`, and
 * every other escape at index `i` is `[value]` alone, copied `i`
 * times -- `i` itself is never used as a byte value in the escape
 * table, which is what makes this unambiguous. Escape 1 doubles as a
 * sequence delimiter unless the "no sequences" flag is set: a first
 * pass then expands each `escape1 ... escape1 [count]` run into
 * `count` copies of the bytes between the two markers, before the
 * escape codes above are applied to what remains.
 *
 * @return The decoded bytes, or an empty vector if `data` is
 *         truncated or its pass id is not 1.
 */
std::vector<std::uint8_t> DecodeStuntsRle(const std::uint8_t* data,
                                          std::size_t size);

}  // namespace sdl3cpp::services::impl

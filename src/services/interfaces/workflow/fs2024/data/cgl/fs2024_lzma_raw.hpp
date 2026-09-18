#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// Decodes one headerless LZMA1 stream, as FS2024's CGL containers
/// store them: no `.lzma` header, the classic lc/lp/pb packed into a
/// single `props` byte ((pb * 5 + lp) * 9 + lc) kept in the container
/// header instead, and the output size known up front from its tile
/// table rather than from an end marker.
std::vector<std::uint8_t> DecodeLzmaRaw(const std::uint8_t* data,
                                        std::size_t size,
                                        std::uint8_t props,
                                        std::size_t outputSize);

/// The same, for a stream whose output size isn't stored anywhere
/// (a CGL's own tile table): decodes until the input runs out.
std::vector<std::uint8_t> DecodeLzmaRawAll(const std::uint8_t* data,
                                           std::size_t size,
                                           std::uint8_t props);

}  // namespace sdl3cpp::fs2024

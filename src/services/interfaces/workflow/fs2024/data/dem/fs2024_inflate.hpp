#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::fs2024 {

/// Inflates a zlib-wrapped DEFLATE buffer (the 2-byte zlib header GTA5's
/// RSC7 reader deliberately skips past -- TIFF's Compression=8 tiles
/// keep it) into exactly `decodedSize` bytes. Throws std::runtime_error
/// on any zlib error, or if the output isn't exactly that size.
std::vector<std::uint8_t> InflateZlib(const std::uint8_t* compressed,
                                      std::size_t compressedSize,
                                      std::size_t decodedSize);

}  // namespace sdl3cpp::fs2024

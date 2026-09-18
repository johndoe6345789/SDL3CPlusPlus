#include "services/interfaces/workflow/fs2024/data/dem/fs2024_inflate.hpp"

#include <zlib.h>

#include <stdexcept>
#include <string>

namespace sdl3cpp::fs2024 {

std::vector<std::uint8_t> InflateZlib(const std::uint8_t* compressed,
                                      std::size_t compressedSize,
                                      std::size_t decodedSize) {
    std::vector<std::uint8_t> out(decodedSize);

    z_stream z{};
    // Positive windowBits: expect the zlib wrapper (2-byte header, 4-byte
    // Adler32 trailer) TIFF's Compression=8 tiles carry, unlike RSC7's
    // headerless raw deflate.
    if (inflateInit2(&z, 15) != Z_OK) {
        throw std::runtime_error("InflateZlib: inflateInit2 failed");
    }
    z.next_in = const_cast<Bytef*>(compressed);
    z.avail_in = static_cast<uInt>(compressedSize);
    z.next_out = out.data();
    z.avail_out = static_cast<uInt>(out.size());

    const int result = inflate(&z, Z_FINISH);
    const std::size_t produced = out.size() - z.avail_out;
    inflateEnd(&z);

    if (result != Z_STREAM_END || produced != decodedSize) {
        throw std::runtime_error("InflateZlib: expected " +
                                 std::to_string(decodedSize) +
                                 " bytes, got " + std::to_string(produced));
    }
    return out;
}

}  // namespace sdl3cpp::fs2024

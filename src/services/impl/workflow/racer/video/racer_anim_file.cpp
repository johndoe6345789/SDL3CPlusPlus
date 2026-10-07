#include "services/interfaces/workflow/racer/video/racer_video_decoder.hpp"

#include <zlib.h>

namespace sdl3cpp::services::impl {

std::optional<std::vector<std::uint8_t>> ReadRacerAnim(
    const std::filesystem::path& path) {
    std::error_code error;
    if (!std::filesystem::is_regular_file(path, error)) return std::nullopt;
    gzFile file = gzopen(path.string().c_str(), "rb");
    if (!file) return std::nullopt;
    std::vector<std::uint8_t> bytes;
    std::uint8_t chunk[1 << 16];
    for (;;) {
        const int read = gzread(file, chunk, sizeof(chunk));
        if (read <= 0) break;
        bytes.insert(bytes.end(), chunk, chunk + read);
    }
    gzclose(file);
    if (bytes.size() < 8) return std::nullopt;
    return bytes;
}

}  // namespace sdl3cpp::services::impl

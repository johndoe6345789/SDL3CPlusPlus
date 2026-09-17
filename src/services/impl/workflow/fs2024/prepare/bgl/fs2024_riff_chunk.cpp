#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_riff_chunk.hpp"

#include <cstring>

namespace sdl3cpp::tools::fs2024 {

std::vector<RiffChunk> WalkRiffChunks(const std::uint8_t* data,
                                     std::size_t size) {
    std::vector<RiffChunk> chunks;
    std::size_t pos = 0;
    while (pos + 8 <= size) {
        std::uint32_t length = 0;
        std::memcpy(&length, data + pos + 4, sizeof(length));
        if (pos + 8 + length > size) break;

        RiffChunk chunk;
        chunk.id.assign(reinterpret_cast<const char*>(data + pos), 4);
        chunk.data = data + pos + 8;
        chunk.size = length;
        chunks.push_back(chunk);

        pos += 8 + length + (length & 1);
    }
    return chunks;
}

}  // namespace sdl3cpp::tools::fs2024

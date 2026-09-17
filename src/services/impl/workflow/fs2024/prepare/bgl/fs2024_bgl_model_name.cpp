#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_bgl_model_name.hpp"

#include "services/interfaces/workflow/fs2024/prepare/bgl/fs2024_riff_chunk.hpp"

namespace sdl3cpp::tools::fs2024 {

std::string HexGuid(const std::uint8_t* bytes16) {
    static const char kHex[] = "0123456789abcdef";
    std::string out(32, '0');
    for (int i = 0; i < 16; ++i) {
        out[i * 2] = kHex[bytes16[i] >> 4];
        out[i * 2 + 1] = kHex[bytes16[i] & 0xF];
    }
    return out;
}

std::string NameFromGxmlPeek(const std::vector<std::uint8_t>& riffHead) {
    if (riffHead.size() < 12 || riffHead[8] != 'G') return {};
    const auto chunks =
        WalkRiffChunks(riffHead.data() + 12, riffHead.size() - 12);
    for (const auto& chunk : chunks) {
        if (chunk.id != "GXML") continue;
        const std::string xml(reinterpret_cast<const char*>(chunk.data),
                              chunk.size);
        const auto at = xml.find("name=\"");
        if (at == std::string::npos) return {};
        const auto start = at + 6;
        const auto end = xml.find('"', start);
        if (end == std::string::npos) return {};
        return xml.substr(start, end - start);
    }
    return {};
}

}  // namespace sdl3cpp::tools::fs2024

#include "services/interfaces/workflow/racer/data/racer_wav.hpp"

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace sdl3cpp::services::impl {
namespace {

std::uint32_t ReadLe32(const std::vector<std::uint8_t>& data,
                       std::size_t at) {
    return data[at] | (data[at + 1] << 8) | (data[at + 2] << 16) |
           (static_cast<std::uint32_t>(data[at + 3]) << 24);
}

std::uint16_t ReadLe16(const std::vector<std::uint8_t>& data,
                       std::size_t at) {
    return static_cast<std::uint16_t>(data[at] | (data[at + 1] << 8));
}

}  // namespace

bool ReadRacerWav(const std::vector<std::uint8_t>& file, RacerPcm& out) {
    if (file.size() < 12 || std::memcmp(&file[0], "RIFF", 4) != 0 ||
        std::memcmp(&file[8], "WAVE", 4) != 0) {
        return false;
    }
    bool haveFormat = false;
    std::size_t pos = 12;
    while (pos + 8 <= file.size()) {
        const std::size_t size = ReadLe32(file, pos + 4);
        const std::size_t body = pos + 8;
        if (body + size > file.size()) return false;
        if (std::memcmp(&file[pos], "fmt ", 4) == 0 && size >= 16) {
            if (ReadLe16(file, body) != 1) return false;  // PCM only
            out.channels = ReadLe16(file, body + 2);
            out.sampleRate = ReadLe32(file, body + 4);
            if (ReadLe16(file, body + 14) != 16) return false;
            haveFormat = true;
        } else if (std::memcmp(&file[pos], "data", 4) == 0 && haveFormat) {
            if (out.channels == 0) return false;
            out.samples.resize(size / 2);
            for (std::size_t i = 0; i < out.samples.size(); ++i) {
                out.samples[i] = static_cast<std::int16_t>(
                    ReadLe16(file, body + 2 * i));
            }
            return true;
        }
        pos = body + size + (size & 1);
    }
    return false;
}

}  // namespace sdl3cpp::services::impl

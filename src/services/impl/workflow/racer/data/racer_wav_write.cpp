#include "services/interfaces/workflow/racer/data/racer_wav.hpp"

#include <cstdint>

namespace sdl3cpp::services::impl {
namespace {

void AppendLe32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<std::uint8_t>(value >> shift));
    }
}

void AppendLe16(std::vector<std::uint8_t>& out, std::uint16_t value) {
    out.push_back(static_cast<std::uint8_t>(value));
    out.push_back(static_cast<std::uint8_t>(value >> 8));
}

}  // namespace

std::vector<std::uint8_t> WriteRacerWav(const RacerPcm& pcm) {
    const std::uint32_t dataBytes =
        static_cast<std::uint32_t>(pcm.samples.size() * 2);
    std::vector<std::uint8_t> out;
    out.reserve(44 + dataBytes);
    out.insert(out.end(), {'R', 'I', 'F', 'F'});
    AppendLe32(out, 36 + dataBytes);
    out.insert(out.end(), {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '});
    AppendLe32(out, 16);
    AppendLe16(out, 1);
    AppendLe16(out, pcm.channels);
    AppendLe32(out, pcm.sampleRate);
    AppendLe32(out, pcm.sampleRate * pcm.channels * 2);
    AppendLe16(out, static_cast<std::uint16_t>(pcm.channels * 2));
    AppendLe16(out, 16);
    out.insert(out.end(), {'d', 'a', 't', 'a'});
    AppendLe32(out, dataBytes);
    for (std::int16_t sample : pcm.samples) {
        AppendLe16(out, static_cast<std::uint16_t>(sample));
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

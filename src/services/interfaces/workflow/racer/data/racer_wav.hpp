#pragma once

#include <cstdint>
#include <vector>

namespace sdl3cpp::services::impl {

/// 16-bit PCM audio, samples interleaved by channel.
struct RacerPcm {
    std::uint16_t channels = 1;
    std::uint32_t sampleRate = 22050;
    std::vector<std::int16_t> samples;
};

/// Parses a RIFF/WAVE file holding 16-bit PCM. Returns false for any
/// other layout, including truncated chunks.
bool ReadRacerWav(const std::vector<std::uint8_t>& file, RacerPcm& out);

/// Writes a canonical 44-byte-header 16-bit PCM WAVE file.
std::vector<std::uint8_t> WriteRacerWav(const RacerPcm& pcm);

/// Linear-interpolation resampling of every channel to `targetRate`.
RacerPcm ResampleRacerPcm(const RacerPcm& pcm, std::uint32_t targetRate);

}  // namespace sdl3cpp::services::impl

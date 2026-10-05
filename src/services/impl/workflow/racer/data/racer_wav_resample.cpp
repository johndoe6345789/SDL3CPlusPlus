#include "services/interfaces/workflow/racer/data/racer_wav.hpp"

#include <algorithm>
#include <cstddef>

namespace sdl3cpp::services::impl {

RacerPcm ResampleRacerPcm(const RacerPcm& pcm, std::uint32_t targetRate) {
    RacerPcm out;
    out.channels = pcm.channels;
    out.sampleRate = targetRate;
    if (pcm.channels == 0 || pcm.sampleRate == 0 || pcm.samples.empty()) {
        return out;
    }
    const std::size_t inFrames = pcm.samples.size() / pcm.channels;
    const std::size_t outFrames = static_cast<std::size_t>(
        inFrames * static_cast<std::uint64_t>(targetRate) / pcm.sampleRate);
    const double step = static_cast<double>(pcm.sampleRate) / targetRate;
    out.samples.resize(outFrames * pcm.channels);
    for (std::size_t frame = 0; frame < outFrames; ++frame) {
        const double pos = frame * step;
        const std::size_t base = static_cast<std::size_t>(pos);
        const std::size_t next = std::min(base + 1, inFrames - 1);
        const double frac = pos - static_cast<double>(base);
        for (std::size_t c = 0; c < pcm.channels; ++c) {
            const double a = pcm.samples[base * pcm.channels + c];
            const double b = pcm.samples[next * pcm.channels + c];
            const double value = a + (b - a) * frac;
            out.samples[frame * pcm.channels + c] = static_cast<std::int16_t>(
                std::clamp(value, -32768.0, 32767.0));
        }
    }
    return out;
}

}  // namespace sdl3cpp::services::impl

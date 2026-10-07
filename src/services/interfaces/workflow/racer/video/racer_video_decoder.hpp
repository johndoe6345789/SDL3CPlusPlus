#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace sdl3cpp::services::impl {

/// Reads one of the install's cutscenes (data/anims/*.znm): a gzip
/// wrapped around a LucasArts SMUSH ('SANM') stream. Empty if missing.
std::optional<std::vector<std::uint8_t>> ReadRacerAnim(
    const std::filesystem::path& path);

/// Decodes a SMUSH stream with FFmpeg: Blocky16 video (640 x 272 at
/// 15 fps) to RGBA, and VIMA ADPCM audio to 16-bit stereo.
class RacerVideoDecoder {
public:
    RacerVideoDecoder();
    ~RacerVideoDecoder();
    RacerVideoDecoder(const RacerVideoDecoder&) = delete;
    RacerVideoDecoder& operator=(const RacerVideoDecoder&) = delete;

    /// Takes the stream's bytes. False if FFmpeg cannot read it.
    bool Open(std::vector<std::uint8_t> bytes);
    int Width() const;
    int Height() const;
    double FramesPerSecond() const;
    int SampleRate() const;   ///< of TakeAudio's samples, 0 if silent

    /// Decodes on to the next picture, as width x height RGBA. False at
    /// the end of the stream.
    bool NextFrame(std::vector<std::uint8_t>& rgba);

    /// Audio decoded so far (interleaved stereo), handed over once.
    std::vector<std::int16_t> TakeAudio();

    /// FFmpeg's state, opaque here (racer_video_impl.hpp).
    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

}  // namespace sdl3cpp::services::impl

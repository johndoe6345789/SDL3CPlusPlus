#pragma once

#include "services/interfaces/workflow/graphics/video_frame.hpp"

#include <memory>
#include <string>

namespace sdl3cpp::services::impl {

/**
 * @brief H.264 through libavcodec, muxed by libavformat.
 *
 * Not thread-safe: the encode worker owns one and is its only caller.
 * Frames of another size are scaled to the size the file has.
 */
class VideoEncoder {
public:
    VideoEncoder();
    ~VideoEncoder();
    VideoEncoder(const VideoEncoder&)            = delete;
    VideoEncoder& operator=(const VideoEncoder&) = delete;

    /// @return "" on success, otherwise why it failed.
    std::string Open(const VideoEncoderSettings& settings);
    bool Write(const VideoFrame& frame);
    /// Interleaved float samples, which follow on from the last ones.
    /// Ignored when the file has no audio track.
    bool WriteAudio(const float* samples, int frames, int rate,
                    int channels);
    /// Flushes the encoder and writes the trailer. Safe to repeat.
    void Close();
    /// The encoder libavcodec picked, e.g. "libx264".
    std::string CodecName() const;

    struct Impl;

private:
    std::unique_ptr<Impl> impl_;
};

}  // namespace sdl3cpp::services::impl

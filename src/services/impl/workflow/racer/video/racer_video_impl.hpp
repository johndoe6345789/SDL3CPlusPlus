#pragma once

#include "services/interfaces/workflow/racer/video/racer_video_decoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace sdl3cpp::services::impl {

/// FFmpeg's state for one cutscene, reading from memory.
struct RacerVideoDecoder::Impl {
    ~Impl();

    std::vector<std::uint8_t> bytes;
    std::int64_t position = 0;           ///< read cursor into `bytes`
    AVIOContext* io = nullptr;
    AVFormatContext* format = nullptr;
    AVCodecContext* video = nullptr;
    AVCodecContext* audio = nullptr;
    int videoStream = -1;
    int audioStream = -1;
    SwsContext* scaler = nullptr;
    SwrContext* resampler = nullptr;
    AVFrame* frame = nullptr;
    AVPacket* packet = nullptr;
    std::vector<std::int16_t> pendingAudio;
    bool ended = false;
};

/// Memory-backed reads and seeks for FFmpeg's AVIOContext.
int RacerVideoRead(void* opaque, std::uint8_t* buffer, int size);
std::int64_t RacerVideoSeek(void* opaque, std::int64_t offset, int whence);

/// Opens a decoder for stream `index` of `format`; null if it cannot.
AVCodecContext* OpenRacerVideoCodec(AVFormatContext* format, int index);

/// Converts a decoded audio frame to 16-bit stereo, appended to `out`.
void ResampleRacerVideoAudio(RacerVideoDecoder::Impl& impl,
                             const AVFrame* frame);

}  // namespace sdl3cpp::services::impl

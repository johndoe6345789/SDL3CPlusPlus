#pragma once

#include "services/interfaces/workflow/graphics/video_encoder.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/audio_fifo.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

#include <string>

namespace sdl3cpp::services::impl {

struct VideoEncoder::Impl {
    AVFormatContext* format = nullptr;
    AVCodecContext* codec   = nullptr;
    AVStream* stream        = nullptr;
    AVFrame* frame          = nullptr;
    AVPacket* packet        = nullptr;
    SwsContext* scaler      = nullptr;
    std::int64_t lastPts    = -1;
    bool headerWritten      = false;
    // The audio track, when the settings ask for one.
    AVCodecContext* audioCodec = nullptr;
    AVStream* audioStream      = nullptr;
    AVFrame* audioFrame        = nullptr;
    AVAudioFifo* audioFifo     = nullptr;
    SwrContext* resampler      = nullptr;
    int resamplerRate          = 0;  // what the resampler takes in
    int resamplerChannels      = 0;
    std::int64_t audioPts      = 0;  // samples encoded so far
};

// The Open* steps return "" on success, otherwise why they failed.
std::string OpenVideoCodec(VideoEncoder::Impl& impl,
                           const VideoEncoderSettings& settings);
std::string OpenAudioCodec(VideoEncoder::Impl& impl,
                           const VideoEncoderSettings& settings);
std::string OpenVideoFile(VideoEncoder::Impl& impl,
                          const std::string& path);
std::string AllocateVideoFrame(VideoEncoder::Impl& impl);
std::string AllocateAudioFrame(VideoEncoder::Impl& impl);

/// Size, rate and x264 options, on a freshly allocated codec context.
void ConfigureVideoCodec(VideoEncoder::Impl& impl,
                         const VideoEncoderSettings& settings);
/// A resampler from @p rate / @p channels float to the audio codec's.
bool EnsureResampler(VideoEncoder::Impl& impl, int rate, int channels);
/// Encodes whole frames from the audio FIFO; @p last pads the rest.
bool EncodeAudioFrames(VideoEncoder::Impl& impl, bool last);
/// Silence up to where the video ends, so the tracks end together.
void PadAudioToVideo(VideoEncoder::Impl& impl);
/// Moves every packet @p codec has ready into @p stream.
bool DrainPackets(VideoEncoder::Impl& impl, AVCodecContext* codec,
                  AVStream* stream);
/// Frees whatever Open got as far as creating.
void FreeVideoEncoder(VideoEncoder::Impl& impl);
/// libav's error text for @p code.
std::string AvErrorText(int code);

}  // namespace sdl3cpp::services::impl

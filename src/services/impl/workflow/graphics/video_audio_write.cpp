#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

bool VideoEncoder::WriteAudio(const float* samples, int frames, int rate,
                              int channels) {
    Impl& impl = *impl_;
    if (!impl.headerWritten || !impl.audioCodec || frames <= 0) {
        return false;
    }
    if (!EnsureResampler(impl, rate, channels)) return false;

    const AVCodecContext& c = *impl.audioCodec;
    const int room          = swr_get_out_samples(impl.resampler, frames);
    uint8_t** planes        = nullptr;
    if (av_samples_alloc_array_and_samples(&planes, nullptr,
                                           c.ch_layout.nb_channels, room,
                                           c.sample_fmt, 0) < 0) {
        return false;
    }
    const uint8_t* in = reinterpret_cast<const uint8_t*>(samples);
    const int got = swr_convert(impl.resampler, planes, room, &in, frames);
    if (got > 0) {
        av_audio_fifo_write(impl.audioFifo,
                            reinterpret_cast<void**>(planes), got);
    }
    av_freep(&planes[0]);
    av_freep(&planes);
    return got >= 0 && EncodeAudioFrames(impl, false);
}

}  // namespace sdl3cpp::services::impl

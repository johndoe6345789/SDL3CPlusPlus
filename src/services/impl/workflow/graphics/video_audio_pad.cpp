#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {

// A quiet game, or none of its audio devices seen, still gives a track
// as long as the picture: players stop at the shorter one.
void PadAudioToVideo(VideoEncoder::Impl& impl) {
    if (!impl.audioCodec || impl.lastPts < 0) return;
    const AVCodecContext& c = *impl.audioCodec;
    const std::int64_t end =
        av_rescale_q(impl.lastPts + 1, impl.codec->time_base, c.time_base);
    const int chunk = impl.audioFrame->nb_samples;

    uint8_t** quiet = nullptr;
    if (av_samples_alloc_array_and_samples(&quiet, nullptr,
                                           c.ch_layout.nb_channels, chunk,
                                           c.sample_fmt, 0) < 0) {
        return;
    }
    av_samples_set_silence(quiet, 0, chunk, c.ch_layout.nb_channels,
                           c.sample_fmt);
    for (;;) {
        const std::int64_t have =
            impl.audioPts + av_audio_fifo_size(impl.audioFifo);
        if (have >= end) break;
        const int n = int(std::min<std::int64_t>(chunk, end - have));
        av_audio_fifo_write(impl.audioFifo, reinterpret_cast<void**>(quiet),
                            n);
        if (!EncodeAudioFrames(impl, false)) break;
    }
    av_freep(&quiet[0]);
    av_freep(&quiet);
}

}  // namespace sdl3cpp::services::impl

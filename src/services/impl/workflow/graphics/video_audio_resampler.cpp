#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

// Kept while the input stays the same: the game's device format is
// fixed once it opens, but a device change can alter it.
bool EnsureResampler(VideoEncoder::Impl& impl, int rate, int channels) {
    if (impl.resampler && impl.resamplerRate == rate &&
        impl.resamplerChannels == channels) {
        return true;
    }
    swr_free(&impl.resampler);
    const AVCodecContext& c = *impl.audioCodec;
    AVChannelLayout in      = {};
    av_channel_layout_default(&in, channels);
    const int err = swr_alloc_set_opts2(
        &impl.resampler, &c.ch_layout, c.sample_fmt, c.sample_rate, &in,
        AV_SAMPLE_FMT_FLT, rate, 0, nullptr);
    av_channel_layout_uninit(&in);
    if (err < 0 || swr_init(impl.resampler) < 0) {
        swr_free(&impl.resampler);
        return false;
    }
    impl.resamplerRate     = rate;
    impl.resamplerChannels = channels;
    return true;
}

}  // namespace sdl3cpp::services::impl

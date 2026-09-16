#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

std::string AllocateAudioFrame(VideoEncoder::Impl& impl) {
    if (!impl.audioCodec) return "";
    const AVCodecContext& c = *impl.audioCodec;
    const int samples       = c.frame_size > 0 ? c.frame_size : 1024;

    impl.audioFrame = av_frame_alloc();
    if (!impl.audioFrame) return "out of memory";
    impl.audioFrame->format      = c.sample_fmt;
    impl.audioFrame->sample_rate = c.sample_rate;
    impl.audioFrame->nb_samples  = samples;
    av_channel_layout_copy(&impl.audioFrame->ch_layout, &c.ch_layout);
    if (int err = av_frame_get_buffer(impl.audioFrame, 0); err < 0) {
        return "audio av_frame_get_buffer: " + AvErrorText(err);
    }

    impl.audioFifo = av_audio_fifo_alloc(
        c.sample_fmt, c.ch_layout.nb_channels, samples * 8);
    return impl.audioFifo ? "" : "av_audio_fifo_alloc failed";
}

}  // namespace sdl3cpp::services::impl

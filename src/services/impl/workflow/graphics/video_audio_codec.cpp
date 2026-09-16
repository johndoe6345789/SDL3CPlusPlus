#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {
namespace {

std::string AddAudioStream(VideoEncoder::Impl& impl) {
    impl.audioStream = avformat_new_stream(impl.format, nullptr);
    if (!impl.audioStream) return "avformat_new_stream failed";
    impl.audioStream->time_base = impl.audioCodec->time_base;
    const int err               = avcodec_parameters_from_context(
        impl.audioStream->codecpar, impl.audioCodec);
    return err < 0 ? "audio parameters: " + AvErrorText(err) : "";
}

}  // namespace

/// FFmpeg's own AAC encoder: in every build, and both containers
/// take it.
std::string OpenAudioCodec(VideoEncoder::Impl& impl,
                           const VideoEncoderSettings& settings) {
    if (settings.audioRate <= 0 || settings.audioChannels <= 0) {
        return "";
    }
    const AVCodec* aac = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (!aac) return "this FFmpeg build has no AAC encoder";
    impl.audioCodec = avcodec_alloc_context3(aac);
    if (!impl.audioCodec) return "avcodec_alloc_context3 failed";

    AVCodecContext& c = *impl.audioCodec;
    c.sample_fmt      = AV_SAMPLE_FMT_FLTP;
    c.sample_rate     = settings.audioRate;
    c.bit_rate        = 160000;
    c.time_base       = AVRational{1, settings.audioRate};
    av_channel_layout_default(&c.ch_layout, settings.audioChannels);
    if (impl.format->oformat->flags & AVFMT_GLOBALHEADER) {
        c.flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if (int err = avcodec_open2(&c, aac, nullptr); err < 0) {
        return "avcodec_open2(aac): " + AvErrorText(err);
    }
    return AddAudioStream(impl);
}

}  // namespace sdl3cpp::services::impl

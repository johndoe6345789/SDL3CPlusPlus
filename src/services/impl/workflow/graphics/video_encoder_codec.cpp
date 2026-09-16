#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {
namespace {

const AVCodec* FindH264Encoder() {
    if (const AVCodec* x264 = avcodec_find_encoder_by_name("libx264")) {
        return x264;
    }
    return avcodec_find_encoder(AV_CODEC_ID_H264);
}

std::string AddStream(VideoEncoder::Impl& impl) {
    impl.stream = avformat_new_stream(impl.format, nullptr);
    if (!impl.stream) return "avformat_new_stream failed";
    impl.stream->time_base = impl.codec->time_base;
    const int err =
        avcodec_parameters_from_context(impl.stream->codecpar, impl.codec);
    return err < 0 ? "codec parameters: " + AvErrorText(err) : "";
}

}  // namespace

/// Opens an H.264 encoder, libx264 by preference, and its stream.
std::string OpenVideoCodec(VideoEncoder::Impl& impl,
                           const VideoEncoderSettings& settings) {
    const AVCodec* codec = FindH264Encoder();
    if (!codec) return "this FFmpeg build has no H.264 encoder";
    impl.codec = avcodec_alloc_context3(codec);
    if (!impl.codec) return "avcodec_alloc_context3 failed";
    ConfigureVideoCodec(impl, settings);

    if (int err = avcodec_open2(impl.codec, codec, nullptr); err < 0) {
        return std::string("avcodec_open2(") + codec->name +
               "): " + AvErrorText(err);
    }
    return AddStream(impl);
}

}  // namespace sdl3cpp::services::impl

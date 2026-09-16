#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

std::string OpenVideoFile(VideoEncoder::Impl& impl,
                          const std::string& path) {
    if (!(impl.format->oformat->flags & AVFMT_NOFILE)) {
        const int err =
            avio_open(&impl.format->pb, path.c_str(), AVIO_FLAG_WRITE);
        if (err < 0) {
            return "cannot write " + path + ": " + AvErrorText(err);
        }
    }
    if (int err = avformat_write_header(impl.format, nullptr); err < 0) {
        return "avformat_write_header: " + AvErrorText(err);
    }
    impl.headerWritten = true;
    return "";
}

std::string AllocateVideoFrame(VideoEncoder::Impl& impl) {
    impl.frame  = av_frame_alloc();
    impl.packet = av_packet_alloc();
    if (!impl.frame || !impl.packet) return "out of memory";
    impl.frame->format = impl.codec->pix_fmt;
    impl.frame->width  = impl.codec->width;
    impl.frame->height = impl.codec->height;
    const int err      = av_frame_get_buffer(impl.frame, 0);
    return err < 0 ? "av_frame_get_buffer: " + AvErrorText(err) : "";
}

}  // namespace sdl3cpp::services::impl

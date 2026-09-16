#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

bool VideoEncoder::Write(const VideoFrame& in) {
    Impl& impl = *impl_;
    // The same pts twice, or an older one, is no frame a file can hold.
    if (!impl.headerWritten || in.pts <= impl.lastPts) return false;

    const AVCodecContext& c = *impl.codec;
    const auto from         = in.bgra ? AV_PIX_FMT_BGRA : AV_PIX_FMT_RGBA;
    impl.scaler             = sws_getCachedContext(
        impl.scaler, in.width, in.height, from, c.width, c.height,
        c.pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!impl.scaler || av_frame_make_writable(impl.frame) < 0) {
        return false;
    }

    const std::uint8_t* planes[1] = {in.pixels.get()};
    const int strides[1]          = {in.width * 4};
    sws_scale(impl.scaler, planes, strides, 0, in.height, impl.frame->data,
              impl.frame->linesize);

    impl.frame->pts = in.pts;
    impl.lastPts    = in.pts;
    if (avcodec_send_frame(impl.codec, impl.frame) < 0) return false;
    return DrainPackets(impl, impl.codec, impl.stream);
}

}  // namespace sdl3cpp::services::impl

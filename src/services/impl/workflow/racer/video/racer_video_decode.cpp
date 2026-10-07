#include "racer_video_impl.hpp"

namespace sdl3cpp::services::impl {
namespace {

/// The decoded picture as RGBA, via a scaler made on first use.
void ToRgba(RacerVideoDecoder::Impl& d, std::vector<std::uint8_t>& rgba) {
    const AVFrame* f = d.frame;
    d.scaler = sws_getCachedContext(
        d.scaler, f->width, f->height, static_cast<AVPixelFormat>(f->format),
        f->width, f->height, AV_PIX_FMT_RGBA, SWS_POINT, nullptr, nullptr,
        nullptr);
    if (!d.scaler) return;
    rgba.resize(static_cast<std::size_t>(f->width) * f->height * 4);
    std::uint8_t* planes[1] = {rgba.data()};
    const int strides[1] = {f->width * 4};
    sws_scale(d.scaler, f->data, f->linesize, 0, f->height, planes, strides);
}

/// Decodes the queued audio packets' frames into the pending samples.
void DrainAudio(RacerVideoDecoder::Impl& d) {
    while (avcodec_receive_frame(d.audio, d.frame) == 0) {
        ResampleRacerVideoAudio(d, d.frame);
    }
}

}  // namespace

bool RacerVideoDecoder::NextFrame(std::vector<std::uint8_t>& rgba) {
    Impl& d = *impl_;
    if (!d.video || d.ended) return false;
    for (;;) {
        // A picture already decoded and waiting comes first.
        if (avcodec_receive_frame(d.video, d.frame) == 0) {
            ToRgba(d, rgba);
            return true;
        }
        if (av_read_frame(d.format, d.packet) < 0) {
            d.ended = true;
            return false;
        }
        if (d.packet->stream_index == d.videoStream) {
            avcodec_send_packet(d.video, d.packet);
        } else if (d.audio && d.packet->stream_index == d.audioStream) {
            if (avcodec_send_packet(d.audio, d.packet) == 0) DrainAudio(d);
        }
        av_packet_unref(d.packet);
    }
}

}  // namespace sdl3cpp::services::impl

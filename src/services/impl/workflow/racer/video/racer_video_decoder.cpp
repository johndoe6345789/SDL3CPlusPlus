#include "racer_video_impl.hpp"

#include <utility>

namespace sdl3cpp::services::impl {
namespace {

constexpr int kIoBufferBytes = 1 << 16;

}  // namespace

RacerVideoDecoder::Impl::~Impl() {
    if (scaler) sws_freeContext(scaler);
    if (resampler) swr_free(&resampler);
    if (frame) av_frame_free(&frame);
    if (packet) av_packet_free(&packet);
    if (video) avcodec_free_context(&video);
    if (audio) avcodec_free_context(&audio);
    if (format) avformat_close_input(&format);
    if (io) {
        av_freep(&io->buffer);
        avio_context_free(&io);
    }
}

RacerVideoDecoder::RacerVideoDecoder() : impl_(std::make_unique<Impl>()) {}
RacerVideoDecoder::~RacerVideoDecoder() = default;

bool RacerVideoDecoder::Open(std::vector<std::uint8_t> bytes) {
    impl_ = std::make_unique<Impl>();
    Impl& d = *impl_;
    d.bytes = std::move(bytes);
    auto* buffer = static_cast<unsigned char*>(av_malloc(kIoBufferBytes));
    d.io = avio_alloc_context(buffer, kIoBufferBytes, 0, &d, RacerVideoRead,
                              nullptr, RacerVideoSeek);
    d.format = avformat_alloc_context();
    if (!buffer || !d.io || !d.format) return false;
    d.format->pb = d.io;
    // The stream is FFmpeg's "smush" format (LucasArts SANM).
    const AVInputFormat* smush = av_find_input_format("smush");
    if (avformat_open_input(&d.format, nullptr, smush, nullptr) < 0) {
        d.format = nullptr;  // freed by FFmpeg on failure
        return false;
    }
    if (avformat_find_stream_info(d.format, nullptr) < 0) return false;
    d.videoStream = av_find_best_stream(d.format, AVMEDIA_TYPE_VIDEO, -1, -1,
                                        nullptr, 0);
    d.audioStream = av_find_best_stream(d.format, AVMEDIA_TYPE_AUDIO, -1, -1,
                                        nullptr, 0);
    d.video = OpenRacerVideoCodec(d.format, d.videoStream);
    d.audio = OpenRacerVideoCodec(d.format, d.audioStream);
    d.frame = av_frame_alloc();
    d.packet = av_packet_alloc();
    return d.video && d.frame && d.packet;
}

int RacerVideoDecoder::Width() const {
    return impl_->video ? impl_->video->width : 0;
}

int RacerVideoDecoder::Height() const {
    return impl_->video ? impl_->video->height : 0;
}

double RacerVideoDecoder::FramesPerSecond() const {
    if (impl_->videoStream < 0) return 15.0;
    const AVRational rate =
        impl_->format->streams[impl_->videoStream]->avg_frame_rate;
    return rate.num > 0 && rate.den > 0 ? av_q2d(rate) : 15.0;
}

int RacerVideoDecoder::SampleRate() const {
    return impl_->audio ? impl_->audio->sample_rate : 0;
}

std::vector<std::int16_t> RacerVideoDecoder::TakeAudio() {
    return std::exchange(impl_->pendingAudio, {});
}

}  // namespace sdl3cpp::services::impl

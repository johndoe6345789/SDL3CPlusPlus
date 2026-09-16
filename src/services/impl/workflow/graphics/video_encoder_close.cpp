#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {
namespace {

void FlushAudio(VideoEncoder::Impl& impl) {
    if (!impl.audioCodec) return;
    PadAudioToVideo(impl);
    EncodeAudioFrames(impl, true);
    avcodec_send_frame(impl.audioCodec, nullptr);
    DrainPackets(impl, impl.audioCodec, impl.audioStream);
}

}  // namespace

VideoEncoder::VideoEncoder() : impl_(std::make_unique<Impl>()) {}

VideoEncoder::~VideoEncoder() {
    Close();
}

void VideoEncoder::Close() {
    if (impl_->headerWritten) {
        avcodec_send_frame(impl_->codec, nullptr);
        DrainPackets(*impl_, impl_->codec, impl_->stream);
        FlushAudio(*impl_);
        av_write_trailer(impl_->format);
    }
    FreeVideoEncoder(*impl_);
}

std::string VideoEncoder::CodecName() const {
    return impl_->codec ? impl_->codec->codec->name : "";
}

}  // namespace sdl3cpp::services::impl

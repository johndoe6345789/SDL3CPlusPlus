#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

void FreeVideoEncoder(VideoEncoder::Impl& impl) {
    if (impl.format && !(impl.format->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&impl.format->pb);
    }
    avformat_free_context(impl.format);
    avcodec_free_context(&impl.codec);
    avcodec_free_context(&impl.audioCodec);
    av_frame_free(&impl.frame);
    av_frame_free(&impl.audioFrame);
    av_packet_free(&impl.packet);
    if (impl.audioFifo) av_audio_fifo_free(impl.audioFifo);
    sws_freeContext(impl.scaler);
    swr_free(&impl.resampler);
    impl = VideoEncoder::Impl{};
}

}  // namespace sdl3cpp::services::impl

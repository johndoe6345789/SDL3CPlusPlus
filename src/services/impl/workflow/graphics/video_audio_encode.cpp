#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

// AAC codes a fixed number of samples a frame; the FIFO holds what
// does not yet make one. The last, short frame is padded out.
bool EncodeAudioFrames(VideoEncoder::Impl& impl, bool last) {
    AVCodecContext* c = impl.audioCodec;
    AVFrame* f        = impl.audioFrame;
    const int size    = f->nb_samples;
    for (;;) {
        const int held = av_audio_fifo_size(impl.audioFifo);
        if (held < size && !(last && held > 0)) return true;
        if (av_frame_make_writable(f) < 0) return false;
        const int got = av_audio_fifo_read(
            impl.audioFifo, reinterpret_cast<void**>(f->data), size);
        if (got < size) {
            av_samples_set_silence(f->data, got, size - got,
                                   c->ch_layout.nb_channels, c->sample_fmt);
        }
        f->pts = impl.audioPts;
        impl.audioPts += size;
        if (avcodec_send_frame(c, f) < 0) return false;
        if (!DrainPackets(impl, c, impl.audioStream)) return false;
    }
}

}  // namespace sdl3cpp::services::impl

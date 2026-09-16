#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

bool DrainPackets(VideoEncoder::Impl& impl, AVCodecContext* codec,
                  AVStream* stream) {
    AVPacket* packet = impl.packet;
    for (;;) {
        const int got = avcodec_receive_packet(codec, packet);
        if (got == AVERROR(EAGAIN) || got == AVERROR_EOF) return true;
        if (got < 0) return false;
        // libx264 leaves the duration unset. The mp4 muxer then gives
        // the last frame none, and its edit list cuts that frame off.
        if (packet->duration <= 0 && codec == impl.codec) {
            packet->duration = 1;
        }
        av_packet_rescale_ts(packet, codec->time_base, stream->time_base);
        packet->stream_index = stream->index;
        // Takes the packet's reference, and leaves the packet blank.
        if (av_interleaved_write_frame(impl.format, packet) < 0) {
            return false;
        }
    }
}

}  // namespace sdl3cpp::services::impl

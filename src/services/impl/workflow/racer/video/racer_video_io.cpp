#include "racer_video_impl.hpp"

#include <algorithm>
#include <cstring>

namespace sdl3cpp::services::impl {

int RacerVideoRead(void* opaque, std::uint8_t* buffer, int size) {
    auto& impl = *static_cast<RacerVideoDecoder::Impl*>(opaque);
    const std::int64_t left =
        static_cast<std::int64_t>(impl.bytes.size()) - impl.position;
    if (left <= 0) return AVERROR_EOF;
    const int count = static_cast<int>(std::min<std::int64_t>(size, left));
    std::memcpy(buffer, impl.bytes.data() + impl.position, count);
    impl.position += count;
    return count;
}

std::int64_t RacerVideoSeek(void* opaque, std::int64_t offset, int whence) {
    auto& impl = *static_cast<RacerVideoDecoder::Impl*>(opaque);
    const auto size = static_cast<std::int64_t>(impl.bytes.size());
    if (whence & AVSEEK_SIZE) return size;
    std::int64_t to = offset;
    if ((whence & 3) == SEEK_CUR) to += impl.position;
    if ((whence & 3) == SEEK_END) to += size;
    if (to < 0 || to > size) return -1;
    impl.position = to;
    return to;
}

AVCodecContext* OpenRacerVideoCodec(AVFormatContext* format, int index) {
    if (index < 0) return nullptr;
    const AVCodecParameters* params = format->streams[index]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(params->codec_id);
    if (!codec) return nullptr;
    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (!context) return nullptr;
    if (avcodec_parameters_to_context(context, params) < 0 ||
        avcodec_open2(context, codec, nullptr) < 0) {
        avcodec_free_context(&context);
        return nullptr;
    }
    return context;
}

void ResampleRacerVideoAudio(RacerVideoDecoder::Impl& impl,
                             const AVFrame* frame) {
    if (!impl.resampler) {
        AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
        if (swr_alloc_set_opts2(&impl.resampler, &stereo, AV_SAMPLE_FMT_S16,
                                frame->sample_rate, &frame->ch_layout,
                                static_cast<AVSampleFormat>(frame->format),
                                frame->sample_rate, 0, nullptr) < 0 ||
            swr_init(impl.resampler) < 0) {
            return;
        }
    }
    const int capacity = swr_get_out_samples(impl.resampler, frame->nb_samples);
    if (capacity <= 0) return;
    const std::size_t start = impl.pendingAudio.size();
    impl.pendingAudio.resize(start + 2 * static_cast<std::size_t>(capacity));
    auto* out = reinterpret_cast<std::uint8_t*>(&impl.pendingAudio[start]);
    const int made = swr_convert(impl.resampler, &out, capacity,
                                 frame->extended_data, frame->nb_samples);
    impl.pendingAudio.resize(start + 2 * static_cast<std::size_t>(
                                             std::max(0, made)));
}

}  // namespace sdl3cpp::services::impl

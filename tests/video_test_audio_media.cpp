#include "video_test_audio_media.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include <algorithm>
#include <cmath>

namespace video_test {

std::vector<float> Tone(int rate, int channels, double seconds) {
    const int frames = int(rate * seconds);
    std::vector<float> out(std::size_t(frames) * channels);
    for (int i = 0; i < frames; ++i) {
        const float v =
            0.5f * float(std::sin(i * 2 * 3.14159 * 440 / rate));
        for (int c = 0; c < channels; ++c) {
            out[i * channels + c] = v;
        }
    }
    return out;
}

AudioSummary ReadAudioBack(const std::string& path) {
    AudioSummary got;
    AVFormatContext* f = nullptr;
    if (avformat_open_input(&f, path.c_str(), nullptr, nullptr) < 0) {
        return got;
    }
    avformat_find_stream_info(f, nullptr);
    got.streams = int(f->nb_streams);
    const int audio =
        av_find_best_stream(f, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (audio >= 0) {
        const AVStream* s = f->streams[audio];
        got.audioChannels = s->codecpar->ch_layout.nb_channels;
        AVPacket* p       = av_packet_alloc();
        while (av_read_frame(f, p) >= 0) {
            const bool shown = !(p->flags & AV_PKT_FLAG_DISCARD);
            if (p->stream_index == audio && shown) {
                const double end =
                    (p->pts + p->duration) * av_q2d(s->time_base);
                got.audioEnd = std::max(got.audioEnd, end);
            }
            av_packet_unref(p);
        }
        av_packet_free(&p);
    }
    avformat_close_input(&f);
    return got;
}

}  // namespace video_test

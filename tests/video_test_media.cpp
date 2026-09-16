#include "video_test_media.hpp"

extern "C" {
#include <libavformat/avformat.h>
}

#include <algorithm>
#include <cmath>
#include <memory>

namespace video_test {

VideoFrame Scrolling(int width, int height, std::int64_t pts) {
    const int bytes    = width * height * 4;
    auto pixels        = std::make_unique<std::uint8_t[]>(bytes);
    std::uint32_t seed = 1;
    for (int i = 0; i < bytes; ++i) {
        seed        = seed * 1103515245u + 12345u;
        const int x = i / 4 % width + int(pts) * 3;
        const int y = i / 4 / width;
        pixels[i]   = std::uint8_t((x * 7 ^ y * 13) + (seed >> 28));
    }
    return {std::move(pixels), width, height, true, pts};
}

Summary ReadBack(const std::string& path) {
    AVFormatContext* f = nullptr;
    if (avformat_open_input(&f, path.c_str(), nullptr, nullptr) < 0) {
        return {-1, -1, -1};
    }
    Summary got        = {0, f->streams[0]->codecpar->width, 0};
    const double frame = av_q2d(f->streams[0]->time_base) * 30.0;
    AVPacket* p        = av_packet_alloc();
    while (av_read_frame(f, p) >= 0) {
        ++got[0];
        const long long end = llround((p->pts + p->duration) * frame);
        // A packet an mp4 edit list cuts is read, but never shown.
        const bool shown = !(p->flags & AV_PKT_FLAG_DISCARD);
        if (shown) got[2] = std::max(got[2], end);
        av_packet_unref(p);
    }
    av_packet_free(&p);
    avformat_close_input(&f);
    return got;
}

}  // namespace video_test

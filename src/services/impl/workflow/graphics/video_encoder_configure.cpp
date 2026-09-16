#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

extern "C" {
#include <libavutil/opt.h>
}

namespace sdl3cpp::services::impl {

void ConfigureVideoCodec(VideoEncoder::Impl& impl,
                         const VideoEncoderSettings& settings) {
    AVCodecContext& c = *impl.codec;
    c.width           = settings.width;
    c.height          = settings.height;
    c.time_base       = AVRational{1, settings.fps};
    c.framerate       = AVRational{settings.fps, 1};
    c.pix_fmt         = AV_PIX_FMT_YUV420P;
    c.gop_size        = settings.fps * 2;
    c.max_b_frames    = 2;
    if (impl.format->oformat->flags & AVFMT_GLOBALHEADER) {
        c.flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    // Only libx264 knows these; other encoders ignore them.
    av_opt_set(c.priv_data, "preset", "veryfast", 0);
    av_opt_set_int(c.priv_data, "crf", settings.quality, 0);
}

}  // namespace sdl3cpp::services::impl

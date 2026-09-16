#include "services/impl/workflow/graphics/video_encoder_impl.hpp"

namespace sdl3cpp::services::impl {

std::string VideoEncoder::Open(const VideoEncoderSettings& requested) {
    VideoEncoderSettings settings = requested;
    settings.width &= ~1;  // 4:2:0 needs even sides
    settings.height &= ~1;
    if (settings.width <= 0 || settings.height <= 0 || settings.fps <= 0) {
        return "bad size or frame rate";
    }
    avformat_alloc_output_context2(&impl_->format, nullptr, nullptr,
                                   settings.path.c_str());
    if (!impl_->format) return "no container for " + settings.path;

    // Every stream is added before the file's header is written.
    std::string error = OpenVideoCodec(*impl_, settings);
    if (error.empty()) error = OpenAudioCodec(*impl_, settings);
    if (error.empty()) error = OpenVideoFile(*impl_, settings.path);
    if (error.empty()) error = AllocateVideoFrame(*impl_);
    if (error.empty()) error = AllocateAudioFrame(*impl_);
    if (!error.empty()) FreeVideoEncoder(*impl_);
    return error;
}

}  // namespace sdl3cpp::services::impl

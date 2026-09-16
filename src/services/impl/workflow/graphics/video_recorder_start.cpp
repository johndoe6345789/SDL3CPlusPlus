#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"
#include "services/interfaces/workflow/graphics/graphics_screenshot_request_helpers.hpp"

#include <string>

namespace sdl3cpp::services::impl {
namespace {

VideoEncoderSettings EncoderSettings(const VideoRecorder& rec,
                                     const WorkflowContext& context) {
    const double scale = rec.settings.scale;
    const auto side    = [&](const char* key) {
        return int(context.Get<uint32_t>(key, 0) * scale);
    };
    VideoEncoderSettings enc;
    enc.path    = ResolveScreenshotOutputPath(rec.settings.path);
    enc.width   = side("frame_width");
    enc.height  = side("frame_height");
    enc.fps     = rec.settings.fps;
    enc.quality = rec.settings.quality;
    if (rec.settings.audio) {  // what the device plays is resampled
        enc.audioRate     = 48000;
        enc.audioChannels = 2;
    }
    return enc;
}

std::string Describe(const VideoEncoderSettings& enc,
                     const std::string& codec) {
    return enc.path + " (" + codec + ", " + std::to_string(enc.width) +
           "x" + std::to_string(enc.height) + " @ " +
           std::to_string(enc.fps) + " fps)";
}

}  // namespace

bool StartVideoRecorder(VideoRecorder& rec, WorkflowContext& context) {
    const VideoEncoderSettings enc = EncoderSettings(rec, context);
    rec.device = context.Get<SDL_GPUDevice*>("gpu_device", nullptr);
    if (rec.settings.audio) {
        rec.audio = std::make_shared<VideoAudioTap>();
        rec.worker.SetAudio(rec.audio);
    }

    const std::string error = rec.worker.Start(enc);
    rec.started             = error.empty();
    rec.done                = !rec.started;
    if (!rec.logger) return rec.started;
    if (rec.started) {
        rec.logger->Info("video.record: recording " +
                         Describe(enc, rec.worker.CodecName()));
    } else {
        rec.logger->Error("video.record: " + error);
    }
    return rec.started;
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

#include <SDL3/SDL_timer.h>

namespace sdl3cpp::services::impl {
namespace {

void Finish(VideoRecorder& rec, WorkflowContext& context) {
    StopVideoRecorder(rec, context);
    if (rec.settings.exitWhenDone) {
        context.Set<bool>(rec.settings.runningKey, false);
    }
}

}  // namespace

std::int64_t DueVideoPts(VideoRecorder& rec, WorkflowContext& context) {
    const std::uint64_t now = SDL_GetTicksNS();
    if (rec.startNs == 0) {
        rec.startNs = now + std::uint64_t(rec.settings.delay * 1e9);
    }
    if (now < rec.startNs) return -1;
    StartVideoAudio(rec);

    const double elapsed = double(now - rec.startNs) / 1e9;
    const double seconds = rec.settings.seconds;
    if (seconds > 0.0 && elapsed >= seconds) {
        Finish(rec, context);
        return -1;
    }
    // A hitch leaves a gap in the timestamps: the frame before it
    // stays on screen longer, as it did in the game.
    const auto pts = std::int64_t(elapsed * rec.settings.fps);
    return pts < rec.nextPts ? -1 : pts;
}

}  // namespace sdl3cpp::services::impl

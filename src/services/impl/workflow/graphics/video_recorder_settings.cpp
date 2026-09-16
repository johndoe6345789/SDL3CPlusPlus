#include "services/interfaces/workflow/graphics/video_recorder_settings.hpp"
#include "services/interfaces/workflow/graphics/video_step_params.hpp"

#include <algorithm>

namespace sdl3cpp::services::impl {
namespace {

double Clamped(const WorkflowStepDefinition& step, const char* name,
               const char* env, double fallback, double lo, double hi) {
    return std::clamp(VideoStepNumber(step, name, env, fallback), lo, hi);
}

}  // namespace

VideoRecorderSettings ReadVideoRecorderSettings(
    const WorkflowStepDefinition& step) {
    const auto& p      = step;
    const double kLong = 1e9;  // seconds: no useful upper bound
    VideoRecorderSettings s;
    s.path = VideoStepString(p, "output_path", "SDL3CPP_RECORD", "");
    s.delay =
        Clamped(p, "start_after", "SDL3CPP_RECORD_DELAY", 0, 0, kLong);
    s.seconds =
        Clamped(p, "seconds", "SDL3CPP_RECORD_SECONDS", 0, 0, kLong);
    s.fps     = int(Clamped(p, "fps", "SDL3CPP_RECORD_FPS", 30, 1, 240));
    s.scale   = Clamped(p, "scale", "SDL3CPP_RECORD_SCALE", 1, 0.1, 1);
    s.quality = int(Clamped(p, "quality", "SDL3CPP_RECORD_CRF", 23, 0, 51));
    s.audio   = VideoStepFlag(p, "audio", "SDL3CPP_RECORD_AUDIO", true);
    s.exitWhenDone =
        VideoStepFlag(p, "exit_when_done", "SDL3CPP_RECORD_EXIT");
    s.runningKey = VideoStepString(p, "running_key", nullptr, s.runningKey);
    return s;
}

}  // namespace sdl3cpp::services::impl

#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

namespace sdl3cpp::services::impl {

VideoRecorder& GetVideoRecorder(WorkflowContext& context,
                                const WorkflowStepDefinition& step,
                                const std::shared_ptr<ILogger>& logger) {
    using Ptr = std::shared_ptr<VideoRecorder>;
    if (const Ptr* found = context.TryGet<Ptr>(kVideoRecorderKey)) {
        return **found;
    }
    // std::any needs a copyable value, and the recorder is not one.
    auto rec      = std::make_shared<VideoRecorder>();
    rec->settings = ReadVideoRecorderSettings(step);
    rec->logger   = logger;
    rec->done     = rec->settings.path.empty();
    context.Set<Ptr>(kVideoRecorderKey, rec);
    return *rec;
}

}  // namespace sdl3cpp::services::impl

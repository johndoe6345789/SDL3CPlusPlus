#include "services/interfaces/workflow/graphics/workflow_video_record_stop_step.hpp"
#include "services/interfaces/workflow/graphics/video_recorder_ops.hpp"

namespace sdl3cpp::services::impl {

WorkflowVideoRecordStopStep::WorkflowVideoRecordStopStep(
    std::shared_ptr<ILogger> logger)
    : logger_(std::move(logger)) {}

std::string WorkflowVideoRecordStopStep::GetPluginId() const {
    return "video.record.stop";
}

// After the frame loop, while the GPU device is still there: the file
// is only playable once its trailer is written.
void WorkflowVideoRecordStopStep::Execute(
    const WorkflowStepDefinition& step, WorkflowContext& context) {
    StopVideoRecorder(GetVideoRecorder(context, step, logger_), context);
}

}  // namespace sdl3cpp::services::impl
